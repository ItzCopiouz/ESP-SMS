/*
 * http_client.c - sending photos to the server
 * 
 * stuff it does:
 * - retries if it fails
 * - sends heartbeats so we know the cam is alive
 */

#include "http_client.h"
#include "config.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "wifi.h"

#include <string.h>
#include <stdio.h>

/* tag for the logs */
static const char *TAG = "HTTP";

/* retry settings */
#define MAX_RETRIES         3
#define INITIAL_BACKOFF_MS  1000   /* start with 1 sec */
#define MAX_BACKOFF_MS      8000   /* cap it at 8 secs */

/*
 * helper: try the request and retry if it fails
 */
static bool http_perform_with_retry(esp_http_client_handle_t client, const char *description)
{
    int backoff_ms = INITIAL_BACKOFF_MS;
    
    for (int attempt = 1; attempt <= MAX_RETRIES; attempt++) {
        ESP_LOGI(TAG, "%s: attempt %d/%d", description, attempt, MAX_RETRIES);
        
        esp_err_t err = esp_http_client_perform(client);
        
        if (err == ESP_OK) {
            int status = esp_http_client_get_status_code(client);
            ESP_LOGI(TAG, "server said: %d", status);
            
            if (status == 200) {
                return true;
            }
            
            /* server error (5xx) - worth another shot */
            if (status >= 500 && status < 600 && attempt < MAX_RETRIES) {
                ESP_LOGW(TAG, "server is acting up (%d), retrying in %d ms...", status, backoff_ms);
                vTaskDelay(pdMS_TO_TICKS(backoff_ms));
                backoff_ms = (backoff_ms * 2 > MAX_BACKOFF_MS) ? MAX_BACKOFF_MS : backoff_ms * 2;
                continue;
            }
            
            /* client error (4xx) - we messed up, don't bother retrying */
            if (status >= 400 && status < 500) {
                ESP_LOGE(TAG, "we messed up the request (%d), giving up", status);
                return false;
            }
        } else {
            ESP_LOGE(TAG, "request failed: %s", esp_err_to_name(err));
            
            if (attempt < MAX_RETRIES) {
                ESP_LOGW(TAG, "retrying in %d ms...", backoff_ms);
                vTaskDelay(pdMS_TO_TICKS(backoff_ms));
                backoff_ms = (backoff_ms * 2 > MAX_BACKOFF_MS) ? MAX_BACKOFF_MS : backoff_ms * 2;
                continue;
            }
        }
    }
    
    ESP_LOGE(TAG, "%s failed for good after %d attempts", description, MAX_RETRIES);
    return false;
}

bool http_post_image(const uint8_t *image_data, size_t image_length)
{
    ESP_LOGI(TAG, "uploading %zu bytes to %s", image_length, CONFIG_BACKEND_URL);
    
    /* keep an eye on memory before the big upload */
    size_t free_heap = esp_get_free_heap_size();
    ESP_LOGI(TAG, "free memory: %zu bytes", free_heap);
    
    esp_http_client_config_t config = {
        .url = CONFIG_BACKEND_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 30000,  /* give it 30s for slow connections */
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "couldn't init the http client");
        return false;
    }

    esp_http_client_set_header(client, "Content-Type", "image/jpeg");
    esp_http_client_set_header(client, "X-Device-Id", CONFIG_DEVICE_ID);
    
    esp_http_client_set_post_field(client, (const char*)image_data, image_length);

    bool success = http_perform_with_retry(client, "uploading photo");

    esp_http_client_cleanup(client);
    
    ESP_LOGI(TAG, "free memory after upload: %zu bytes", esp_get_free_heap_size());
    return success;
}

bool http_post_heartbeat(int battery_mv, int battery_percent, int uptime_ms)
{
    /* figure out the heartbeat url */
    char heartbeat_url[256];
    
    /* swap /capture for /heartbeat in the url */
    strncpy(heartbeat_url, CONFIG_BACKEND_URL, sizeof(heartbeat_url) - 1);
    char *capture = strstr(heartbeat_url, "/capture");
    if (capture != NULL) {
        strcpy(capture, "/heartbeat");
    } else {
        /* fallback just in case */
        strncat(heartbeat_url, "/../heartbeat", sizeof(heartbeat_url) - strlen(heartbeat_url) - 1);
    }
    
    ESP_LOGI(TAG, "sending heartbeat to %s", heartbeat_url);
    
    /* build the json payload */
    char json_payload[256];
    int wifi_rssi = wifi_get_rssi();
    size_t free_heap = esp_get_free_heap_size();
    
    snprintf(json_payload, sizeof(json_payload),
        "{\"battery_voltage\":%d,\"battery_percent\":%d,\"wifi_rssi\":%d,\"free_heap\":%zu,\"uptime_ms\":%d}",
        battery_mv, battery_percent, wifi_rssi, free_heap, uptime_ms
    );
    
    esp_http_client_config_t config = {
        .url = heartbeat_url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 10000,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "couldn't init http client for heartbeat");
        return false;
    }
    
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "X-Device-Id", CONFIG_DEVICE_ID);
    esp_http_client_set_post_field(client, json_payload, strlen(json_payload));
    
    bool success = http_perform_with_retry(client, "heartbeat");
    
    esp_http_client_cleanup(client);
    return success;
}
