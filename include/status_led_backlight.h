#pragma once

#include <zephyr/drivers/led_strip.h>
#include <zephyr/sys/util.h>

#include <tincan/tincan.h>

#define STATUS_LED_TINCAN_CHANNEL CONFIG_ZMK_2NDIDEAL_STATUS_LED_TINCAN_CHANNEL

BUILD_ASSERT(STATUS_LED_TINCAN_CHANNEL > TINCAN_CHANNEL_CONTROL &&
                 STATUS_LED_TINCAN_CHANNEL <= TINCAN_CHANNEL_MAX,
             "set CONFIG_ZMK_2NDIDEAL_STATUS_LED_TINCAN_CHANNEL in the board's "
             ".conf, to a value in 1..TINCAN_CHANNEL_MAX (0 is reserved by "
             "zmk-tincan for its HELLO handshake)");

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
