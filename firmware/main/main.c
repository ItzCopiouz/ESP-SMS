/*
 * main.c - Entry point for ESP32-CAM firmware
 * 
 * Flow: boot → WiFi → camera → capture → HTTP POST → deep sleep
 */

#include "esp_log.h"
#include "power.h"
#include "wifi.h"
#include "camera.h"
#include "http_client.h"

static const char *TAG = "MAIN";

/*
 * app_main - Entry point (like Python's if __name__ == "__main__")
 */
void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "ESP32-CAM Starting...");
    ESP_LOGI(TAG, "========================================");

    /* Check if we woke from deep sleep or fresh boot */
    bool from_sleep = power_woke_from_sleep();
    
    /* Step 1: Initialize power (disable LED, set up wake source) */
    if (!power_init()) {
        ESP_LOGE(TAG, "Power initialization failed");
        return;
    }
    
    
    /* Step 2: Connect to WiFi */
    if (!wifi_init_and_connect()) {
        ESP_LOGE(TAG, "WiFi initialization failed");
        return;
    }
    
    /* Step 3: Initialize camera */
    if (!camera_init()) {
        ESP_LOGE(TAG, "Camera initialization failed");
        return;
    }
    
    
    /* Step 4: Capture an image */
    uint8_t *image_data;
    size_t image_length;
    if (!camera_capture(&image_data, &image_length)) {
        ESP_LOGE(TAG, "Failed to capture image");
        camera_deinit();
        wifi_disconnect();
        power_enter_deep_sleep();
    }
    
    
    /* Step 5: Send image to backend */
    bool sent = http_post_image(image_data, image_length);
    if (sent) {
        ESP_LOGI(TAG, "Image sent successfully!");
    } else {
        ESP_LOGW(TAG, "Failed to send image");
    }
    
    
    /* Step 6: Cleanup */
    camera_release_buffer();
    camera_deinit();
    wifi_disconnect();
    
    
    /* Step 7: Enter deep sleep */
    ESP_LOGI(TAG, "All done! Going to sleep...");
    power_enter_deep_sleep();  /* This never returns */
}
