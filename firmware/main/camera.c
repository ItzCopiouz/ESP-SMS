/*
 * camera.c - OV3660 camera initialization and capture
 */

#include "camera.h"
#include "config.h"
#include "esp_log.h"
#include "esp_camera.h"

/* tag for the logs */
static const char *TAG = "CAMERA";

/* hold onto the frame buffer so we can dump it later */
static camera_fb_t *s_fb = NULL;

bool camera_init(void)
{
    /* setup the pins from config.h */
    camera_config_t config = {
        /* data pins */
        .pin_d0 = CAM_PIN_D0,
        .pin_d1 = CAM_PIN_D1,
        .pin_d2 = CAM_PIN_D2,
        .pin_d3 = CAM_PIN_D3,
        .pin_d4 = CAM_PIN_D4,
        .pin_d5 = CAM_PIN_D5,
        .pin_d6 = CAM_PIN_D6,
        .pin_d7 = CAM_PIN_D7,

        /* control pins */
        .pin_xclk = CAM_PIN_XCLK,
        .pin_vsync = CAM_PIN_VSYNC,
        .pin_href = CAM_PIN_HREF,
        .pin_pclk = CAM_PIN_PCLK,

        /* i2c pins (sccb) */
        .pin_sccb_sda = CAM_PIN_SDA,
        .pin_sccb_scl = CAM_PIN_SCL,

        /* power and reset */
        .pin_pwdn = CAM_PIN_PWDN,
        .pin_reset = CAM_PIN_RESET,

        /* clock speed */
        .xclk_freq_hz = CAM_XCLK_FREQ,

        /* frame buffer settings */
        .fb_count = 1,  /* just one frame, we aren't streaming netflix */
        .fb_location = CAMERA_FB_IN_PSRAM,  /* put the big image in PSRAM */
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY,

        /* image settings */
        .pixel_format = PIXFORMAT_JPEG,
        .frame_size = FRAMESIZE_HD,  /* 1280x720 is plenty */
        .jpeg_quality = CAM_JPEG_QUALITY,
    };

    /* fire up the camera */
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "camera failed to start: %s", esp_err_to_name(err));
        return false;
    }

    ESP_LOGI(TAG, "camera ready (HD, quality %d)", CAM_JPEG_QUALITY);
    return true;
}

bool camera_capture(uint8_t **out_buffer, size_t *out_length)
{
    /* grab a frame from the sensor */
    s_fb = esp_camera_fb_get();
    
    if (s_fb == NULL) {
        ESP_LOGE(TAG, "couldn't grab a photo");
        *out_buffer = NULL;
        *out_length = 0;
        return false;
    }

    /* give the caller the goods */
    *out_buffer = s_fb->buf;
    *out_length = s_fb->len;

    ESP_LOGI(TAG, "got the photo: %zu bytes", s_fb->len);
    return true;
}

void camera_release_buffer(void)
{
    /* give the buffer back to the driver */
    if (s_fb != NULL) {
        esp_camera_fb_return(s_fb);
        s_fb = NULL;
        ESP_LOGI(TAG, "buffer released");
    }
}

void camera_deinit(void)
{
    /* clean up before leaving */
    camera_release_buffer();
    
    esp_err_t err = esp_camera_deinit();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "camera deinit was messy: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "camera turned off");
    }
}
