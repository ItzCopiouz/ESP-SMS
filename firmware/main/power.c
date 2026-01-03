/*
 * power.c - sleep mode, waking up, and keeping an eye on the battery
 */

#include "power.h"
#include "config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

/* tag for the logs */
static const char *TAG = "POWER";

/* handles for the battery sensor */
static adc_oneshot_unit_handle_t s_adc_handle = NULL;
static adc_cali_handle_t s_adc_cali_handle = NULL;
static bool s_adc_calibrated = false;

/* tracking how long we've been awake */
static int64_t s_boot_time_us = 0;

/*
 * setup the adc to read battery volts
 * usually uses GPIO36 on these camera boards
 */
static bool init_battery_adc(void)
{
    esp_err_t ret;
    
    /* setup the adc unit */
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    
    ret = adc_oneshot_new_unit(&unit_cfg, &s_adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "adc init failed, no battery info for you");
        return false;
    }
    
    /* setup the channel - GPIO36 is the spot */
    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_12,  /* read up to ~3.3V */
        .bitwidth = ADC_BITWIDTH_12,
    };
    
    ret = adc_oneshot_config_channel(s_adc_handle, BATTERY_ADC_CHANNEL, &chan_cfg);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "adc channel config failed");
        adc_oneshot_del_unit(s_adc_handle);
        s_adc_handle = NULL;
        return false;
    }
    
    /* try to calibrate it so the numbers are actually right */
    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id = ADC_UNIT_1,
        .chan = BATTERY_ADC_CHANNEL,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    
    ret = adc_cali_create_scheme_curve_fitting(&cali_cfg, &s_adc_cali_handle);
    if (ret == ESP_OK) {
        s_adc_calibrated = true;
        ESP_LOGI(TAG, "adc calibration is good to go");
    } else {
        ESP_LOGW(TAG, "no calibration, numbers might be a bit off");
        s_adc_calibrated = false;
    }
    
    ESP_LOGI(TAG, "battery sensor ready on channel %d", BATTERY_ADC_CHANNEL);
    return true;
}

bool power_init(void)
{
    esp_err_t ret;
    
    /* grab the boot time so we can track uptime */
    s_boot_time_us = esp_timer_get_time();

    /* see if we crashed because of low volts last time */
    esp_reset_reason_t reset_reason = esp_reset_reason();
    if (reset_reason == ESP_RST_BROWNOUT) {
        ESP_LOGW(TAG, "!!! BROWNOUT !!! - we died last time because the battery was too low");
    }
    
    /* setup the built-in protection */
    ESP_LOGI(TAG, "brownout protection is on");

    /* kill the bright flash led so we don't blind anyone */
    gpio_reset_pin(FLASH_LED_GPIO);
    gpio_set_direction(FLASH_LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(FLASH_LED_GPIO, 0);
    
    ESP_LOGI(TAG, "flash led is off on GPIO %d", FLASH_LED_GPIO);

    /* setup the button that wakes us up */
    ret = esp_sleep_enable_ext0_wakeup(CONFIG_WAKE_BUTTON_GPIO, 0);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "couldn't setup the wake button: %s", esp_err_to_name(ret));
        return false;
    }

    /* make sure the button pin stays high while we sleep */
    ret = rtc_gpio_pullup_en(CONFIG_WAKE_BUTTON_GPIO);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "failed to enable pullup on GPIO %d", CONFIG_WAKE_BUTTON_GPIO);
    }
    rtc_gpio_pulldown_dis(CONFIG_WAKE_BUTTON_GPIO);

    ESP_LOGI(TAG, "wake button ready on GPIO %d", CONFIG_WAKE_BUTTON_GPIO);
    
    /* fire up the battery sensor */
    init_battery_adc();

    return true;
}

void power_enter_deep_sleep(void)
{
    ESP_LOGI(TAG, "going to sleep... hit the button to wake me up");
    
    /* clean up adc before we pass out */
    if (s_adc_cali_handle != NULL) {
        adc_cali_delete_scheme_curve_fitting(s_adc_cali_handle);
        s_adc_cali_handle = NULL;
    }
    if (s_adc_handle != NULL) {
        adc_oneshot_del_unit(s_adc_handle);
        s_adc_handle = NULL;
    }
    
    /* wait a sec for logs to finish */
    vTaskDelay(pdMS_TO_TICKS(100));

    /* everything resets when we wake up */
    esp_deep_sleep_start();
}

bool power_woke_from_sleep(void)
{
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

    switch (cause) {
        case ESP_SLEEP_WAKEUP_EXT0:
            ESP_LOGI(TAG, "the button woke me up");
            return true;
        
        case ESP_SLEEP_WAKEUP_UNDEFINED:
            ESP_LOGI(TAG, "fresh boot or hard reset");
            return false;

        default:
            ESP_LOGI(TAG, "woke up for some weird reason: %d", cause);
            return false;
    }
}

int power_get_battery_mv(void)
{
    if (s_adc_handle == NULL) {
        return 0;  /* no sensor, no numbers */
    }
    
    int raw_value = 0;
    esp_err_t ret = adc_oneshot_read(s_adc_handle, BATTERY_ADC_CHANNEL, &raw_value);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "couldn't read the battery");
        return 0;
    }
    
    int voltage_mv = 0;
    
    if (s_adc_calibrated && s_adc_cali_handle != NULL) {
        /* use the calibrated math */
        adc_cali_raw_to_voltage(s_adc_cali_handle, raw_value, &voltage_mv);
    } else {
        /* just guess based on raw values */
        voltage_mv = (raw_value * 3300) / 4095;
    }
    
    /* multiply by the divider factor (usually 2 for LiPo boards) */
    voltage_mv = voltage_mv * BATTERY_DIVIDER_FACTOR;
    
    ESP_LOGD(TAG, "battery: raw=%d, voltage=%d mV", raw_value, voltage_mv);
    return voltage_mv;
}

int power_get_battery_percent(void)
{
    int voltage_mv = power_get_battery_mv();
    
    if (voltage_mv == 0) {
        return 0;  /* nothing to report */
    }
    
    /* basic LiPo math:
     * 4.2V is full
     * 3.7V is halfway
     * 3.3V is basically dead
     */
    if (voltage_mv >= 4200) return 100;
    if (voltage_mv <= 3300) return 0;
    
    /* linear guess between dead and full */
    int percent = ((voltage_mv - 3300) * 100) / (4200 - 3300);
    return percent;
}

int power_get_uptime_ms(void)
{
    int64_t now_us = esp_timer_get_time();
    int64_t uptime_us = now_us - s_boot_time_us;
    return (int)(uptime_us / 1000);  /* back to milliseconds */
}

bool power_was_brownout_reset(void)
{
    return esp_reset_reason() == ESP_RST_BROWNOUT;
}
