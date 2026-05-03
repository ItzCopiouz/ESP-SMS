/*
 * http_client.h - backend HTTP client
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Upload a JPEG to the backend capture endpoint. */
bool http_post_image(const uint8_t *image_data, size_t image_length);

/* Send device telemetry to the backend heartbeat endpoint. */
bool http_post_heartbeat(int battery_mv, int battery_percent, int uptime_ms);
