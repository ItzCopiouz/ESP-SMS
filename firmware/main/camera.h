/*
 * camera.h - OV3660 camera initialization and capture
 */

#pragma once

#include <stdbool.h>  /* For bool type */
#include <stddef.h>   /* For size_t type */
#include <stdint.h>   /* For uint8_t type */

/*
 * Initialize the OV3660 camera with fixed settings:
 * - Resolution: VGA (640x480)
 * - Format: JPEG
 * - Quality: 15 (good balance of quality and size)
 * 
 * Returns: true on success, false on failure
 */
bool camera_init(void);

/*
 * Capture a single JPEG image.
 * 
 * out_buffer: Pointer to store the address of the JPEG data
 * out_length: Pointer to store the length of the JPEG data in bytes
 * 
 * Returns: true on success, false on failure
 * 
 * IMPORTANT: You must call camera_release_buffer() after you're done
 * with the image data to free the frame buffer.
 */
bool camera_capture(uint8_t **out_buffer, size_t *out_length);

/*
 * Release the frame buffer back to the camera driver.
 * Call this after you've finished processing/sending the image.
 */
void camera_release_buffer(void);

/*
 * Deinitialize the camera and free resources.
 * Call this before entering deep sleep.
 */
void camera_deinit(void);
