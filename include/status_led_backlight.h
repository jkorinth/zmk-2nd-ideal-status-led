#pragma once

#include <zephyr/drivers/led_strip.h>

/*
 * Turns the backlight pixels (all strip pixels except the status LED) on or
 * off on this half. Implemented once in status_led.c and shared by both the
 * central and peripheral build.
 *
 * Kept out of status_led.h: that header is also pulled into DTS
 * preprocessing (via dts/dt-bindings/status_led.h) and must stay macros-only.
 */
void status_led_set_backlight(int on);

/* Writes a single strip pixel (chain_idx 0 = status LED, 1-6 = underglow)
 * and pushes the whole strip. Used by bl_store.c for live-preview renders on
 * this half. */
void status_led_set_pixel(uint8_t chain_idx, struct led_rgb rgb);
