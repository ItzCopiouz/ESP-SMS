/*
 * config.h - all the settings for the esp32-cam
 */

#pragma once

/* wifi setup */
/* your actual wifi credentials go in wifi_secrets.h (which is gitignored) */
/* check out wifi_secrets.h.example if you need a template */

/* where the server lives */
#ifndef CONFIG_BACKEND_URL
#define CONFIG_BACKEND_URL "https://your-server.com/api/v1/capture"
#endif

/* what we call this specific camera */
#ifndef CONFIG_DEVICE_ID
#define CONFIG_DEVICE_ID "ESP32CAM_001"
#endif

/* pin for the wake-up button */
#ifndef CONFIG_WAKE_BUTTON_GPIO
#define CONFIG_WAKE_BUTTON_GPIO 13
#endif

/* the bright led for the flash */
#define FLASH_LED_GPIO 4

/* camera pin mapping (should be right for most esp32-cam boards) */
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
#define CAM_PIN_RESET  -1  /* not using a reset pin */

/* camera tweaks */
#define CAM_XCLK_FREQ  20000000  /* 20 MHz clock is standard */
#define CAM_JPEG_QUALITY 15      /* 0 to 63 (lower is better quality) */

/* battery monitoring stuff */
/* uses GPIO36 (ADC1 Channel 0) by default */
#define BATTERY_ADC_CHANNEL ADC_CHANNEL_0

/* 
 * if you're using a voltage divider (like 100k+100k), set this to 2.
 * it multiplies the reading to get the real battery voltage.
 */
#define BATTERY_DIVIDER_FACTOR 2

/* all done with config */
