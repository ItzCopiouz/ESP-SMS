/*
 * power.h - Deep sleep and wake-up management
 */

#pragma once

#include <stdbool.h>  /* For bool type */

/*
 * Initialize power management.
 * - Disables the flash LED (GPIO 4)
 * - Sets up wake-up source (button on GPIO 13)
 * 
 * Returns: true on success, false on failure
 */
bool power_init(void);

/*
 * Enter deep sleep mode.
 * Device will wake when the button is pressed.
 * This function does not return bc the device resets on wake.
 */
void power_enter_deep_sleep(void);

/*
 * Check if we woke from deep sleep (vs power-on reset).
 * 
 * Returns: true if woke from deep sleep, false if cold boot
 */
bool power_woke_from_sleep(void);
