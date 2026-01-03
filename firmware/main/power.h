/*
 * power.h - Deep sleep, wake-up, battery monitoring, and brownout handling
 */

#pragma once

#include <stdbool.h>  /* For bool type */

/*
 * Initialize power management.
 * - Checks for brownout reset and logs warning
 * - Disables the flash LED (GPIO 4)
 * - Sets up wake-up source (button on GPIO 13)
 * - Initializes battery ADC if available
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

/*
 * Get battery voltage in millivolts.
 * 
 * Returns: Battery voltage in mV, or 0 if not available
 */
int power_get_battery_mv(void);

/*
 * Get battery percentage (0-100).
 * Uses LiPo discharge curve approximation.
 * 
 * Returns: Battery percentage, or 0 if not available
 */
int power_get_battery_percent(void);

/*
 * Get uptime since boot in milliseconds.
 * 
 * Returns: Milliseconds since power_init() was called
 */
int power_get_uptime_ms(void);

/*
 * Check if last reset was caused by brownout (low voltage).
 * 
 * Returns: true if brownout caused the reset
 */
bool power_was_brownout_reset(void);
