#pragma once

/*
 * dts/dt-bindings/status_led.h #includes this file so the keymap's DTS can
 * see these macros. ZMK's DTS preprocessor runs cpp as `-x assembler-with-cpp
 * -nostdinc` and feeds whatever comes out straight to the devicetree parser -
 * so this file must stay macros-only. Anything else (function declarations,
 * even libc-free ones) belongs in status_led_backlight.h instead.
 */
#define SL_TURN_ON 1
#define SL_TURN_OFF 2
