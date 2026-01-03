/*
 * wifi.h - making sure we have an internet connection
 */

#pragma once

#include <stdbool.h>

/*
 * fire up wifi and try to find a network
 */
bool wifi_init_and_connect(void);

/*
 * shut down wifi before we sleep
 */
void wifi_disconnect(void);

/*
 * how strong is the signal? (dBm)
 */
int wifi_get_rssi(void);
