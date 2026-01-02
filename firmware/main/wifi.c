/*
 * wifi.c - WiFi connection management implementation
 */

#include "wifi.h"
#include "config.h"

/* ESP-IDF includes */
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

/* Tag for logging */
static const char *TAG = "WIFI";

/* Event group to signal when we're connected */
static EventGroupHandle_t s_wifi_event_group;

/* Bit flags for the event group */
#define WIFI_CONNECTED_BIT  BIT0  /* We got an IP address */
#define WIFI_FAIL_BIT       BIT1  /* Connection failed */

/* Retry counter */
#define MAX_RETRY  5
static int s_retry_count = 0;

/*
 * Event handler so like ESP-IDF calls this when WiFi events happen
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data)
{
    /* WiFi events */
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                /* WiFi hardware started - now try to connect */
                ESP_LOGI(TAG, "WiFi started, connecting...");
                esp_wifi_connect();
                break;
                
            case WIFI_EVENT_STA_DISCONNECTED:
                /* We got disconnected - retry or give up */
                if (s_retry_count < MAX_RETRY) {
                    s_retry_count++;
                    ESP_LOGW(TAG, "Disconnected, retrying (%d/%d)...", 
                             s_retry_count, MAX_RETRY);
                    esp_wifi_connect();
                } else {
                    ESP_LOGE(TAG, "Failed to connect after %d attempts", MAX_RETRY);
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

bool wifi_init_and_connect(void)
{
    esp_err_t ret;
    
    /* Step 1: Initialize NVS cus WiFi needs this to store calibration data */
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

    /* Step 8: Configure WiFi with SSID and password */
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    /* Step 9: Start WiFi */
    ESP_LOGI(TAG, "Connecting to '%s'...", CONFIG_WIFI_SSID);
    ESP_ERROR_CHECK(esp_wifi_start());

    /* Step 10: Wait for connection or failure */
    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY
    );

    /* Check result */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "WiFi connected successfully");
        return true;
    } else {
        ESP_LOGE(TAG, "WiFi connection failed");
        return false;
    }
}

void wifi_disconnect(void)
{
    ESP_LOGI(TAG, "Disconnecting WiFi...");
    
    esp_wifi_stop();
    esp_wifi_deinit();
    
    if (s_wifi_event_group != NULL) {
        vEventGroupDelete(s_wifi_event_group);
        s_wifi_event_group = NULL;
    }
    
    ESP_LOGI(TAG, "WiFi disconnected");
}

