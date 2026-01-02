/*
 * wifi.h - WiFi connection management
 */

#pragma once

#include <stdbool.h>  /* For bool type */

/*
 * Initialize WiFi in station mode and connect to the configured network.
 * This function blocks until connected or timeout.
 * 
 * Returns: true if connected successfully, false on failure/timeout
 */
bool wifi_init_and_connect(void);

/*
 * Disconnect from WiFi and free resources.
 * Call this before entering deep sleep to ensure clean shutdown.
 */
void wifi_disconnect(void);
