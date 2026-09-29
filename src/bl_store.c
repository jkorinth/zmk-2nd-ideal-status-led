#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/util.h>

#include <bl_ids.h>
#include <bl_store.h>
#include <status_led_backlight.h>
#include <tincan/tincan.h>
#include <zmk/keymap.h>

#include <pb.h>
#include <pb_encode.h>
#include <proto/si-sl.pb.h>

LOG_MODULE_REGISTER(bl_store, CONFIG_ZMK_LOG_LEVEL);

#define LEDS_PER_HALF 6
#define BL_LED_COUNT (LEDS_PER_HALF * 2)

#define HUE_MAX 360
#define SAT_MAX 100
#define BRT_MAX 100

#define STRIP_BRIGHTNESS CONFIG_ZMK_2NDIDEAL_STATUS_LED_BRIGHTNESS

/* HSB, same convention as vendored zmk/app/src/rgb_underglow.c's
 * zmk_led_hsb: h 0-359, s/b 0-100. */
struct bl_led_hsb {
  uint16_t h;
  uint8_t s;
  uint8_t b;
};

/* Default brightness, as a percentage, rounded to nearest so hsb_to_rgb()
 * reproduces the old flat-fill behavior (white at STRIP_BRIGHTNESS) as
 * closely as the 0-100 percentage representation allows (within ~1/255). */
#define STRIP_BRIGHTNESS_PCT (((STRIP_BRIGHTNESS * 100) + 127) / 255)

/* index 0..5 = this half's chain idx 1..6, index 6..11 = the other half's
 * chain idx 1..6. Persisted as a single blob under settings key "bl/leds".
 * Default: white (s=0) at the same brightness the old flat backlight fill
 * used, so upgrading firmware doesn't change the backlight's look until the
 * user actually tunes an LED. */
static struct bl_led_hsb leds[BL_LED_COUNT] = {
    [0 ... BL_LED_COUNT - 1] = {.h = 0, .s = 0, .b = STRIP_BRIGHTNESS_PCT},
};

struct bl_editor_state {
  bool editing;
  struct bl_led_ref led;
  struct bl_led_hsb scratch; /* live, uncommitted */
};
static struct bl_editor_state editor;

static uint8_t flat_idx(struct bl_led_ref led) {
  return (led.peripheral ? LEDS_PER_HALF : 0) + (led.chain_idx - 1);
}

/* Ported from zmk/app/src/rgb_underglow.c's hsb_to_rgb(), adapted to the
 * local bl_led_hsb type. */
static struct led_rgb hsb_to_rgb(struct bl_led_hsb hsb) {
  float r = 0, g = 0, b = 0;

  uint8_t i = hsb.h / 60;
  float v = hsb.b / ((float)BRT_MAX);
  float s = hsb.s / ((float)SAT_MAX);
  float f = hsb.h / ((float)HUE_MAX) * 6 - i;
  float p = v * (1 - s);
  float q = v * (1 - f * s);
  float t = v * (1 - (1 - f) * s);

  switch (i % 6) {
  case 0:
    r = v;
    g = t;
    b = p;
    break;
  case 1:
    r = q;
    g = v;
    b = p;
    break;
  case 2:
    r = p;
    g = v;
    b = t;
    break;
  case 3:
    r = p;
    g = q;
    b = v;
    break;
  case 4:
    r = t;
    g = p;
    b = v;
    break;
  case 5:
    r = v;
    g = p;
    b = q;
    break;
  }

  return (struct led_rgb){.r = r * 255, .g = g * 255, .b = b * 255};
}

static void send_color(uint8_t chain_idx, struct led_rgb rgb) {
  si_sl_Msg m = si_sl_Msg_init_default;
  m.which_msg = si_sl_Msg_set_color_tag;
  m.msg.set_color.strip_index = chain_idx;
  m.msg.set_color.has_color = true;
  m.msg.set_color.color.r = rgb.r;
  m.msg.set_color.color.g = rgb.g;
  m.msg.set_color.color.b = rgb.b;

  static uint8_t buf[128];
  pb_ostream_t stream = pb_ostream_from_buffer(buf, sizeof(buf));
  if (pb_encode(&stream, si_sl_Msg_fields, &m) == 0) {
    LOG_ERR("failed to encode color message: %s", PB_GET_ERROR(&stream));
  } else {
    tincan_speak(buf, stream.bytes_written);
  }
}

static void render(struct bl_led_ref led, struct bl_led_hsb hsb) {
  struct led_rgb rgb = hsb_to_rgb(hsb);
  if (led.peripheral) {
    send_color(led.chain_idx, rgb);
  } else {
    status_led_set_pixel(led.chain_idx, rgb);
  }
}

void bl_store_select(struct bl_led_ref led) {
  editor.editing = true;
  editor.led = led;
  editor.scratch = leds[flat_idx(led)];
  zmk_keymap_layer_to(BL_ADJUST_LAYER, false);
}

void bl_store_adjust(uint8_t component, int8_t step) {
  if (!editor.editing) {
    return;
  }
  switch (component) {
  case BL_HUE:
    editor.scratch.h = (editor.scratch.h + HUE_MAX + step * 10) % HUE_MAX;
    break;
  case BL_SAT:
    editor.scratch.s = CLAMP((int)editor.scratch.s + step * 5, 0, SAT_MAX);
    break;
  case BL_BRIGHT:
    editor.scratch.b = CLAMP((int)editor.scratch.b + step * 5, 0, BRT_MAX);
    break;
  default:
    LOG_ERR("invalid component: %u", component);
    return;
  }
  render(editor.led, editor.scratch);
}

void bl_store_commit(void) {
  if (!editor.editing) {
    return;
  }
  leds[flat_idx(editor.led)] = editor.scratch;
  settings_save_one("bl/leds", &leds, sizeof(leds));
  editor.editing = false;
  zmk_keymap_layer_to(BL_SELECT_LAYER, false);
}

void bl_store_discard(void) {
  if (!editor.editing) {
    return;
  }
  render(editor.led, leds[flat_idx(editor.led)]);
  editor.editing = false;
  zmk_keymap_layer_to(BL_SELECT_LAYER, false);
}

struct led_rgb bl_store_get_local_rgb(uint8_t chain_idx) {
  return hsb_to_rgb(leds[chain_idx - 1]);
}

void bl_store_push_peripheral_colors(void) {
  si_sl_Msg m = si_sl_Msg_init_default;
  m.which_msg = si_sl_Msg_set_colors_tag;
  m.msg.set_colors.colors_count = LEDS_PER_HALF;
  for (uint8_t i = 0; i < LEDS_PER_HALF; i++) {
    struct led_rgb rgb = hsb_to_rgb(leds[LEDS_PER_HALF + i]);
    m.msg.set_colors.colors[i].strip_index = i + 1;
    m.msg.set_colors.colors[i].has_color = true;
    m.msg.set_colors.colors[i].color.r = rgb.r;
    m.msg.set_colors.colors[i].color.g = rgb.g;
    m.msg.set_colors.colors[i].color.b = rgb.b;
  }

  static uint8_t buf[128];
  pb_ostream_t stream = pb_ostream_from_buffer(buf, sizeof(buf));
  if (pb_encode(&stream, si_sl_Msg_fields, &m) == 0) {
    LOG_ERR("failed to encode colors message: %s", PB_GET_ERROR(&stream));
  } else {
    tincan_speak(buf, stream.bytes_written);
  }
}

#if IS_ENABLED(CONFIG_SETTINGS)
static int bl_store_settings_set(const char *name, size_t len, settings_read_cb read_cb,
                                  void *cb_arg) {
  const char *next;

  if (settings_name_steq(name, "leds", &next) && !next) {
    if (len != sizeof(leds)) {
      return -EINVAL;
    }
    return read_cb(cb_arg, &leds, sizeof(leds)) >= 0 ? 0 : -EINVAL;
  }

  return -ENOENT;
}

SETTINGS_STATIC_HANDLER_DEFINE(bl_store, "bl", NULL, bl_store_settings_set, NULL, NULL);
#endif
