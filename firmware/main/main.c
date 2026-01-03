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

/*
 * app_main - Entry point (like Python's if __name__ == "__main__")
 */
void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "ESP32-CAM is waking up...");
    ESP_LOGI(TAG, "========================================");

    /* see if we just woke up or if this is a fresh start */
    bool from_sleep = power_woke_from_sleep();
    (void)from_sleep;  /* don't really need this yet but keeping it around just in case */
    
    /* step 1: get the power situation sorted (kill the led, setup wake, check for low volts) */
    if (!power_init()) {
        ESP_LOGE(TAG, "power stuff failed, gg bruz");
        return;
    }
    
    /* check if we died last time because the battery was too low */
    if (power_was_brownout_reset()) {
        ESP_LOGW(TAG, "we crashed last time! voltage was too low.");
        ESP_LOGW(TAG, "battery check: %d mV (%d%%)", 
                 power_get_battery_mv(), power_get_battery_percent());
    }
    
    /* step 2: get online */
    if (!wifi_init_and_connect()) {
        ESP_LOGE(TAG, "wifi failed, going back to sleep to try later");
        power_enter_deep_sleep();
        return;
    }
    
    /* step 3: wake up the camera */
    if (!camera_init()) {
        ESP_LOGE(TAG, "camera is acting up");
        /* at least tell the server we're alive before we pass out */
        http_post_heartbeat(power_get_battery_mv(), power_get_battery_percent(), power_get_uptime_ms());
        wifi_disconnect();
        power_enter_deep_sleep();
        return;
    }
    
    /* step 4: grab a photo */
    uint8_t *image_data;
    size_t image_length;
    if (!camera_capture(&image_data, &image_length)) {
        ESP_LOGE(TAG, "couldn't take the pic");
        /* tell the server we're still here */
        http_post_heartbeat(power_get_battery_mv(), power_get_battery_percent(), power_get_uptime_ms());
        camera_deinit();
        wifi_disconnect();
        power_enter_deep_sleep();
        return;
    }
    
    /* Step 5: Send image to backend */
    bool sent = http_post_image(image_data, image_length);
    if (sent) {
        ESP_LOGI(TAG, "photo sent!");
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
