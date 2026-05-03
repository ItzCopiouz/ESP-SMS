/*
 * main.c - Entry point for ESP32-CAM firmware
 * 
 * Flow: boot → WiFi → camera → capture → HTTP POST → heartbeat → deep sleep
 */

#include "esp_log.h"
#include "power.h"
#include "wifi.h"
#include "camera.h"
#include "http_client.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "ESP32-CAM is waking up...");
    ESP_LOGI(TAG, "========================================");

    /* Check whether this boot came from deep sleep or a fresh reset. */
    bool from_sleep = power_woke_from_sleep();
    (void)from_sleep;
    
    if (!power_init()) {
        ESP_LOGE(TAG, "Power initialization failed");
        return;
    }
    
    if (power_was_brownout_reset()) {
        ESP_LOGW(TAG, "Previous reset was caused by brownout");
        ESP_LOGW(TAG, "battery check: %d mV (%d%%)", 
                 power_get_battery_mv(), power_get_battery_percent());
    }
    
    if (!wifi_init_and_connect()) {
        ESP_LOGE(TAG, "WiFi failed; returning to deep sleep");
        power_enter_deep_sleep();
        return;
    }
    
    if (!camera_init()) {
        ESP_LOGE(TAG, "Camera initialization failed");
        http_post_heartbeat(power_get_battery_mv(), power_get_battery_percent(), power_get_uptime_ms());
        wifi_disconnect();
        power_enter_deep_sleep();
        return;
    }
    
    /* step 4: grab a photo */
    uint8_t *image_data;
    size_t image_length;
    if (!camera_capture(&image_data, &image_length)) {
        ESP_LOGE(TAG, "Image capture failed");
        http_post_heartbeat(power_get_battery_mv(), power_get_battery_percent(), power_get_uptime_ms());
        camera_deinit();
        wifi_disconnect();
        power_enter_deep_sleep();
        return;
    }
    
    /* Step 5: Send image to backend */
    bool sent = http_post_image(image_data, image_length);
    if (sent) {
        ESP_LOGI(TAG, "Photo sent");
    } else {
        ESP_LOGW(TAG, "Failed to send image after retries");
    }
    
    /* step 6: send telemetry so we know how the battery is doing */
    int battery_mv = power_get_battery_mv();
    int battery_pct = power_get_battery_percent();
    int uptime_ms = power_get_uptime_ms();
    
    ESP_LOGI(TAG, "Battery: %d mV (%d%%), Uptime: %d ms", battery_mv, battery_pct, uptime_ms);
    
    if (!http_post_heartbeat(battery_mv, battery_pct, uptime_ms)) {
        ESP_LOGW(TAG, "heartbeat failed to send");
    }
    
    /* Step 7: Cleanup */
    camera_release_buffer();
    camera_deinit();
    wifi_disconnect();
    
    /* Step 8: Enter deep sleep */
    ESP_LOGI(TAG, "All done! Going to sleep...");
    power_enter_deep_sleep();  /* This never returns */
}
