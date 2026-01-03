/*
 * config.h - Compile-time configuration with fallback defaults
 * 
 * Values from menuconfig take priority.
 * These defaults are used if menuconfig values aren't set.
 */

#pragma once

/* ========== WiFi Configuration ========== */
/* WiFi credentials are now in wifi_secrets.h (gitignored) */
/* Copy wifi_secrets.h.example to wifi_secrets.h and add your networks */

/* ========== Backend Configuration ========== */
#ifndef CONFIG_BACKEND_URL
#define CONFIG_BACKEND_URL "https://your-server.com/api/v1/capture"
#endif

#ifndef CONFIG_DEVICE_ID
#define CONFIG_DEVICE_ID "ESP32CAM_001"
#endif

/* ========== Hardware Configuration ========== */
#ifndef CONFIG_WAKE_BUTTON_GPIO
#define CONFIG_WAKE_BUTTON_GPIO 13
#endif

/* Flash LED GPIO, supposedly GIPO4 */
#define FLASH_LED_GPIO 4

/* ========== Camera Pin Mapping i think ========== */
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
#define CAM_PIN_RESET  -1  /* Not connected */

/* ========== Camera Settings ========== */
#define CAM_XCLK_FREQ  20000000  /* 20 MHz clock for OV3660 */
#define CAM_JPEG_QUALITY 15      /* 0-63, lower is better quality */

/* End of config.h */

