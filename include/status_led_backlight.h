#pragma once

/*
 * Turns the backlight pixels (all strip pixels except the status LED) on or
 * off on this half. Implemented once in status_led.c and shared by both the
 * central and peripheral build.
 *
 * Kept out of status_led.h: that header is also pulled into DTS
 * preprocessing (via dts/dt-bindings/status_led.h) and must stay macros-only.
 */
void status_led_set_backlight(int on);
