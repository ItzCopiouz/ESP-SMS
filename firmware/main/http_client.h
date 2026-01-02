/*
 * http_client.h - HTTPS client for posting images to backend
 */

#pragma once

#include <stdbool.h>  /* For bool type */
#include <stddef.h>   /* For size_t type */
#include <stdint.h>   /* For uint8_t type */

/*
 * Post a JPEG image to the backend server.
 * 
 * image_data: Pointer to the JPEG bytes
 * image_length: Length of the JPEG data in bytes
 * 
 * Returns: true if server responded with HTTP 200, false otherwise
 * 
 * On failure, this function will retry once before returning false.
 */
bool http_post_image(const uint8_t *image_data, size_t image_length);
