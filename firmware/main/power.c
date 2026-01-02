/*
 * power.c - Deep sleep and wake-up management implementation
 */

#include "power.h"
#include "config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"

/* Tag for logging */
static const char *TAG = "POWER";

bool power_init(void)
{
    esp_err_t ret;

    /*Disable flash LED */
    gpio_reset_pin(FLASH_LED_GPIO);
    gpio_set_direction(FLASH_LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(FLASH_LED_GPIO, 0);
    
    ESP_LOGI(TAG, "Flash LED disabled on GPIO %d", FLASH_LED_GPIO);

    /*set up the wake-up source*/
    ret = esp_sleep_enable_ext0_wakeup(CONFIG_WAKE_BUTTON_GPIO, 0);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure wake up source: %s", esp_err_to_name(ret));
        return false;
    }

    /* Enable internal pull-up resistor */
    ret = rtc_gpio_pullup_en(CONFIG_WAKE_BUTTON_GPIO);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to enable pull up on GPIO %d", CONFIG_WAKE_BUTTON_GPIO);
    }
    rtc_gpio_pulldown_dis(CONFIG_WAKE_BUTTON_GPIO);

    ESP_LOGI(TAG, "Wake source configured on GPIO %d", CONFIG_WAKE_BUTTON_GPIO);

    return true;
}

void power_enter_deep_sleep(void)
{
    ESP_LOGI(TAG, "Entering deep sleep... press button to wake up");
    /* delay for msg to transmit*/
    vTaskDelay(pdMS_TO_TICKS(100));

    /*device resets on wake*/
    esp_deep_sleep_start();
}

bool power_woke_from_sleep(void)
{
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

    switch (cause) {
        case ESP_SLEEP_WAKEUP_EXT0:
            ESP_LOGI(TAG, "Woke from deep sleep by external button press");
            return true;
        
        case ESP_SLEEP_WAKEUP_UNDEFINED:
            ESP_LOGI(TAG, "Fresh boot (power on or reset)");
            return false;

        default:
            ESP_LOGI(TAG, "Woke from an unknown cause: %d", cause);
            return false;
            
    }
}
