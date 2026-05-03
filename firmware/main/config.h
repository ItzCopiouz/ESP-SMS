/*
 * config.h - ESP32-CAM firmware settings
 */

#pragma once

/* WiFi credentials live in wifi_secrets.h. */

#ifndef CONFIG_BACKEND_URL
#define CONFIG_BACKEND_URL "https://esp32.samcc.work/api/v1/capture"
#endif

#ifndef CONFIG_DEVICE_ID
#define CONFIG_DEVICE_ID "ESP32CAM_001"
#endif

#ifndef CONFIG_WAKE_BUTTON_GPIO
#define CONFIG_WAKE_BUTTON_GPIO 13
#endif

#define FLASH_LED_GPIO 4

/* Camera pin mapping for common ESP32-CAM boards. */
#define CAM_PIN_D0     5
#define CAM_PIN_D1     18
#define CAM_PIN_D2     19
#define CAM_PIN_D3     21
#define CAM_PIN_D4     36
#define CAM_PIN_D5     39
#define CAM_PIN_D6     34
#define CAM_PIN_D7     35
#define CAM_PIN_XCLK   0
#define CAM_PIN_PCLK   22
#define CAM_PIN_VSYNC  25
#define CAM_PIN_HREF   23
#define CAM_PIN_SDA    26
#define CAM_PIN_SCL    27
#define CAM_PIN_PWDN   32
#define CAM_PIN_RESET  -1  /* No reset pin. */

/* camera tweaks */
#define CAM_XCLK_FREQ  20000000  /* 20 MHz clock is standard */
#define CAM_JPEG_QUALITY 15      /* 0 to 63 (lower is better quality) */

/* battery monitoring stuff */
/* Uses GPIO36 (ADC1 Channel 0) by default. */
#define BATTERY_ADC_CHANNEL ADC_CHANNEL_0

/* 
 * If using a voltage divider, set this to the divider multiplier.
 * For example, a 100k+100k divider should use 2.
 */
#define BATTERY_DIVIDER_FACTOR 2
