/*
 * wifi.c - WiFi connection management implementation
 * 
 * Supports multiple WiFi credentials - tries each in order until one connects.
 */

#include "wifi.h"
#include "wifi_secrets.h"

/* ESP-IDF includes */
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include <string.h>

/* Tag for logging */
static const char *TAG = "WIFI";

/* Event group to signal when we're connected */
static EventGroupHandle_t s_wifi_event_group;

/* Bit flags for the event group */
#define WIFI_CONNECTED_BIT  BIT0  /* We got an IP address */
#define WIFI_FAIL_BIT       BIT1  /* Connection failed */

/* Retry counter */
#define MAX_RETRY_PER_NETWORK  3
static int s_retry_count = 0;

/*
 * Event handler - ESP-IDF calls this when WiFi events happen
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data)
{
    /* WiFi events */
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                /* WiFi hardware started - now try to connect */
                esp_wifi_connect();
                break;
                
            case WIFI_EVENT_STA_DISCONNECTED:
                /* We got disconnected - retry or give up on this network */
                if (s_retry_count < MAX_RETRY_PER_NETWORK) {
                    s_retry_count++;
                    ESP_LOGW(TAG, "Disconnected, retrying (%d/%d)...", 
                             s_retry_count, MAX_RETRY_PER_NETWORK);
                    esp_wifi_connect();
                } else {
                    /* Max retries for this network - signal failure to try next */
                    xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
                }
                break;
        }
    }
    
    /* IP events - we got an IP address! */
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Connected! IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_count = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/*
 * Try to connect to a specific network
 * Returns true if connected, false if failed after retries
 */
static bool try_connect_to_network(const wifi_cred_t *cred)
{
    /* Reset retry counter for this network */
    s_retry_count = 0;
    
    /* Clear any previous event bits */
    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
    
    /* Configure WiFi with this network's credentials */
    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    
    /* Copy SSID and password (must use strncpy for fixed-size arrays) */
    strncpy((char *)wifi_config.sta.ssid, cred->ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, cred->password, sizeof(wifi_config.sta.password) - 1);
    
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    
    /* Start connection attempt */
    ESP_ERROR_CHECK(esp_wifi_connect());
    
    /* Wait for connection or failure (with timeout) */
    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        pdMS_TO_TICKS(15000)  /* 15 second timeout per network */
    );
    
    if (bits & WIFI_CONNECTED_BIT) {
        return true;
    }
    
    /* Disconnect cleanly before trying next network */
    esp_wifi_disconnect();
    return false;
}

bool wifi_init_and_connect(void)
{
    esp_err_t ret;
    
    /* Step 1: Initialize NVS - WiFi needs this to store calibration data */
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS corrupted, erasing...");
        nvs_flash_erase();
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NVS init failed: %s", esp_err_to_name(ret));
        return false;
    }

    /* Step 2: Create the event group for signaling */
    s_wifi_event_group = xEventGroupCreate();
    if (s_wifi_event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create event group");
        return false;
    }

    /* Step 3: Initialize the TCP/IP network interface */
    ESP_ERROR_CHECK(esp_netif_init());
    
    /* Step 4: Create the default event loop */
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    /* Step 5: Create WiFi station interface */
    esp_netif_create_default_wifi_sta();

    /* Step 6: Initialize WiFi with default config */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* Step 7: Register our event handler */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    /* Step 8: Set WiFi mode and start */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    /* Step 9: Try each network in order */
    ESP_LOGI(TAG, "Trying %d configured WiFi networks...", WIFI_CRED_COUNT);
    
    for (int i = 0; i < WIFI_CRED_COUNT; i++) {
        ESP_LOGI(TAG, "Attempting network %d/%d: '%s'", 
                 i + 1, WIFI_CRED_COUNT, WIFI_CREDENTIALS[i].ssid);
        
        if (try_connect_to_network(&WIFI_CREDENTIALS[i])) {
            ESP_LOGI(TAG, "WiFi connected successfully to '%s'", 
                     WIFI_CREDENTIALS[i].ssid);
            return true;
        }
        
        ESP_LOGW(TAG, "Failed to connect to '%s'", WIFI_CREDENTIALS[i].ssid);
    }

    ESP_LOGE(TAG, "All %d WiFi networks failed", WIFI_CRED_COUNT);
    return false;
}

void wifi_disconnect(void)
{
    ESP_LOGI(TAG, "Disconnecting WiFi...");
    
    esp_wifi_disconnect();
    esp_wifi_stop();
    esp_wifi_deinit();
    
    if (s_wifi_event_group != NULL) {
        vEventGroupDelete(s_wifi_event_group);
        s_wifi_event_group = NULL;
    }
    
    ESP_LOGI(TAG, "WiFi disconnected");
}
