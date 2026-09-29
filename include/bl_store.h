#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/drivers/led_strip.h>

/*
 * Must match config/2nd_ideal.keymap's keymap { } layer order (0-based):
 * default_layer=0, numbers_layer=1, f_layer=2, bl_select_layer=3,
 * bl_adjust_layer=4.
 */
#define BL_SELECT_LAYER 3
#define BL_ADJUST_LAYER 4

struct bl_led_ref {
  bool peripheral;   /* false = this (central/left) half, true = the other (peripheral/right) half */
  uint8_t chain_idx; /* 1..6: this half's local WS2812 chain index / si_sl_SetColor strip_index */
};

/*
 * Enters edit mode for `led`: seeds the live scratch value from storage and
 * switches to BL_ADJUST_LAYER. Central-only, called from behavior_bl_select.c.
 */
void bl_store_select(struct bl_led_ref led);

/*
 * Adjusts one HSB component of the currently-selected LED and immediately
 * re-renders the live preview (a local pixel write, or a SetColor push over
 * tincan for a peripheral-half LED). No-op if nothing is selected. Does not
 * persist. Called from behavior_bl_adjust.c.
 */
void bl_store_adjust(uint8_t component, int8_t step);

/* Persists the in-progress edit and returns to BL_SELECT_LAYER. */
void bl_store_commit(void);

/*
 * Discards the in-progress edit (reverts the live preview to the last
 * persisted value) and returns to BL_SELECT_LAYER.
 */
void bl_store_discard(void);

/*
 * This half's stored color for local chain_idx (1..6), for
 * status_led_set_backlight() to render on backlight-on.
 */
struct led_rgb bl_store_get_local_rgb(uint8_t chain_idx);

/*
 * Pushes this half's stored view of the peripheral's 6 underglow colors to
 * it via a single tincan SetColors message. Called right before every
 * CMD_TURN_ON so the peripheral always has fresh colors, including after its
 * own reboot/reconnect.
 */
void bl_store_push_peripheral_colors(void);
