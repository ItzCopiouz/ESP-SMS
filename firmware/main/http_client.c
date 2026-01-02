/*
 * http_client.c - HTTPS client for posting images to backend
 */

#include "http_client.h"
#include "config.h"
#include "esp_log.h"
#include "esp_http_client.h"

/* Tag for logging */
static const char *TAG = "HTTP";

bool http_post_image(const uint8_t *image_data, size_t image_length)
{
    ESP_LOGI(TAG, "Posting %zu bytes to %s", image_length, CONFIG_BACKEND_URL);
    
    
    esp_http_client_config_t config = {
        .url = CONFIG_BACKEND_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 10000,
    };
    
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return false;
    }

    
    esp_http_client_set_header(client, "Content-Type", "image/jpeg");
    esp_http_client_set_header(client, "X-Device-Id", CONFIG_DEVICE_ID);
    
    esp_http_client_set_post_field(client, (const char*)image_data, image_length);

    esp_err_t err = esp_http_client_perform(client);

    bool success = false;
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP Response: %d", status);
        success = (status == 200);
    } else {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return success;
}
