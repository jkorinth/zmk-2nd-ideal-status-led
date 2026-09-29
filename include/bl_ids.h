#pragma once

/*
 * dts/dt-bindings/bl_ids.h #includes this file so the keymap's DTS can see
 * these macros. Must stay macros-only (see status_led.h for why).
 *
 * bl-select param1: which LED to edit, encoded as (half | chain_idx).
 * chain_idx (1-6) matches this half's WS2812 chain order and si_sl_SetColor's
 * strip_index: 1/5/3 = top row left/middle/right, 2/6/4 = bottom row
 * left/middle/right (see xiao_ble.overlay's led_strip chain order and the
 * underglow schematic's DIN->DOUT daisy chain).
 */
#define BL_HALF_LOCAL      0x00
#define BL_HALF_PERIPHERAL 0x10

#define BL_LED_L_TOP1 (BL_HALF_LOCAL | 1)
#define BL_LED_L_TOP2 (BL_HALF_LOCAL | 5)
#define BL_LED_L_TOP3 (BL_HALF_LOCAL | 3)
#define BL_LED_L_BOT1 (BL_HALF_LOCAL | 2)
#define BL_LED_L_BOT2 (BL_HALF_LOCAL | 6)
#define BL_LED_L_BOT3 (BL_HALF_LOCAL | 4)

#define BL_LED_R_TOP1 (BL_HALF_PERIPHERAL | 1)
#define BL_LED_R_TOP2 (BL_HALF_PERIPHERAL | 5)
#define BL_LED_R_TOP3 (BL_HALF_PERIPHERAL | 3)
#define BL_LED_R_BOT1 (BL_HALF_PERIPHERAL | 2)
#define BL_LED_R_BOT2 (BL_HALF_PERIPHERAL | 6)
#define BL_LED_R_BOT3 (BL_HALF_PERIPHERAL | 4)

/* bl-adjust param1: which HSB component. param2: signed step direction. */
#define BL_HUE    0
#define BL_SAT    1
#define BL_BRIGHT 2
#define BL_STEP_DOWN (-1)
#define BL_STEP_UP   (1)

/* bl-commit param1 */
#define BL_COMMIT  1
#define BL_DISCARD 2
