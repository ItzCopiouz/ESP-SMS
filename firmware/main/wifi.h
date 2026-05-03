/*
 * wifi.h - WiFi connection management
 */

#pragma once

#include <stdbool.h>

/* Initialize WiFi and connect to one of the configured networks. */
bool wifi_init_and_connect(void);

/* Disconnect WiFi before entering deep sleep. */
void wifi_disconnect(void);

/* Return the current RSSI in dBm, or 0 if disconnected. */
int wifi_get_rssi(void);
