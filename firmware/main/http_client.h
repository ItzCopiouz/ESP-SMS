/*
 * http_client.h - talking to the backend server
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * upload a jpeg to the server
 */
bool http_post_image(const uint8_t *image_data, size_t image_length);

/*
 * tell the server we're still alive and send some stats
 */
bool http_post_heartbeat(int battery_mv, int battery_percent, int uptime_ms);
