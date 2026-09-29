#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <status_led_backlight.h>
#include <tincan/tincan.h>
#include <zmk/event_manager.h>

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#include <bl_store.h>
#include <zmk/events/layer_state_changed.h>
#else // !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#include <zephyr/bluetooth/conn.h>
#endif // !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#include <zmk/keymap.h>

LOG_MODULE_REGISTER(status_led, CONFIG_ZMK_LOG_LEVEL);

#define STRIP_NODE DT_CHOSEN(zmk_underglow)
#define STRIP_NUM_PIXELS DT_PROP(STRIP_NODE, chain_length)
#define STRIP_BRIGHTNESS CONFIG_ZMK_2NDIDEAL_STATUS_LED_BRIGHTNESS

static const struct device *const strip = DEVICE_DT_GET(STRIP_NODE);
static struct led_rgb pixels[STRIP_NUM_PIXELS];

typedef enum {
  COLOR_OFF,
  COLOR_OK,
  COLOR_ERR,
  COLOR_SELECT, // bl_select_layer indicator
  COLOR_ADJUST, // bl_adjust_layer indicator
  COLOR_COUNT
} status_led_color_t;

static struct led_rgb colors[COLOR_COUNT] = {
    {.r = 0, .g = 0, .b = 0},                                             // COLOR_OFF
    {.r = STRIP_BRIGHTNESS, .g = STRIP_BRIGHTNESS, .b = STRIP_BRIGHTNESS}, // COLOR_OK
    {.r = 0, .g = 0, .b = STRIP_BRIGHTNESS},                              // COLOR_ERR
    {.r = STRIP_BRIGHTNESS, .g = 0, .b = STRIP_BRIGHTNESS},               // COLOR_SELECT (magenta)
    {.r = STRIP_BRIGHTNESS, .g = STRIP_BRIGHTNESS, .b = 0},               // COLOR_ADJUST (yellow)
};

// pixels[0] is indexed directly by zmk_keymap_highest_layer_active(), so
// colors[] must have one entry per keymap layer or that indexing reads past
// the array. Catch a too-short colors[] at build time...
BUILD_ASSERT(ARRAY_SIZE(colors) >= ZMK_KEYMAP_LAYERS_LEN,
             "status_led colors[] has fewer entries than there are keymap layers");

void status_led_set_backlight(int on) {
#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  for (size_t i = 1; i < STRIP_NUM_PIXELS; i++) {
    pixels[i] = on ? bl_store_get_local_rgb(i) : (struct led_rgb){0};
  }
#else
  if (!on) {
    for (size_t i = 1; i < STRIP_NUM_PIXELS; i++) {
      pixels[i] = (struct led_rgb){0};
    }
  }
  // on: leave pixels[1..STRIP_NUM_PIXELS) as-is — the central pushes fresh
  // per-pixel colors via a SetColors message right before every TURN_ON.
#endif
  led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
}

void status_led_set_pixel(uint8_t chain_idx, struct led_rgb rgb) {
  if (chain_idx >= STRIP_NUM_PIXELS) {
    return;
  }
  pixels[chain_idx] = rgb;
  led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
}

#if IS_ENABLED(CONFIG_ZMK_2NDIDEAL_STATUS_LED_TEST)
static void status_led_test_walk(void) {
  static const struct led_rgb test_colors[3] = {
      {.r = STRIP_BRIGHTNESS, .g = 0, .b = 0},
      {.r = 0, .g = STRIP_BRIGHTNESS, .b = 0},
      {.r = 0, .g = 0, .b = STRIP_BRIGHTNESS},
  };
  LOG_INF("status LED test walk: starting, %u pixels", STRIP_NUM_PIXELS);
  for (size_t i = 0; i < STRIP_NUM_PIXELS; i++) {
    for (size_t c = 0; c < 3; c++) {
      memset(pixels, 0, sizeof(pixels));
      pixels[i] = test_colors[c];
      int ret = led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
      LOG_INF("pixel %u color %u: led_strip_update_rgb() = %d", i, c, ret);
      k_msleep(CONFIG_ZMK_2NDIDEAL_STATUS_LED_TEST_DELAY_MS);
    }
  }
  memset(pixels, 0, sizeof(pixels));
  led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
  LOG_INF("status LED test walk: done");
}
static void status_led_fade_walk_thread(void *p1, void *p2, void *p3) {
  ARG_UNUSED(p1);
  ARG_UNUSED(p2);
  ARG_UNUSED(p3);
  for (;;) {
    for (size_t i = 1; i < STRIP_NUM_PIXELS; i++) {
      for (int b = 0; b <= STRIP_BRIGHTNESS; b++) {
        pixels[i] = (struct led_rgb){.r = b, .g = b, .b = b};
        led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
        k_msleep(CONFIG_ZMK_2NDIDEAL_STATUS_LED_TEST_DELAY_MS);
      }
      for (int b = STRIP_BRIGHTNESS; b >= 0; b--) {
        pixels[i] = (struct led_rgb){.r = b, .g = b, .b = b};
        led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
        k_msleep(CONFIG_ZMK_2NDIDEAL_STATUS_LED_TEST_DELAY_MS);
      }
    }
  }
}

// ponytail: temporary diagnostic, remove once LED wiring is confirmed healthy
K_THREAD_DEFINE(status_led_fade_walk_tid, 512, status_led_fade_walk_thread, NULL,
                 NULL, NULL, K_PRIO_PREEMPT(10), 0, 1000);
#endif // IS_ENABLED(CONFIG_ZMK_2NDIDEAL_STATUS_LED_TEST)

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#include <pb.h>
#include <pb_encode.h>
#include <proto/si-sl.pb.h>

static uint8_t current_layer;

static int status_led_event_listener(const zmk_event_t *eh) {
  uint8_t prev_layer = current_layer;
  current_layer = zmk_keymap_highest_layer_active();
  LOG_DBG("prev layer: %d, curr layer: %d", prev_layer, current_layer);
  if (prev_layer != current_layer) {
    // BUILD_ASSERT above guards colors[] against being too short at compile
    // time; this is the runtime fallback (e.g. layers added dynamically past
    // what colors[] was sized for) — falls back to COLOR_OFF rather than
    // reading past the array.
    pixels[0] = colors[current_layer >= ARRAY_SIZE(colors) ? COLOR_OFF : current_layer];
    led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);

    si_sl_Msg m = si_sl_Msg_init_default;
    m.which_msg = si_sl_Msg_set_color_tag;
    m.msg.set_color.strip_index = 0;
    m.msg.set_color.color.r = pixels[0].r;
    m.msg.set_color.color.g = pixels[0].g;
    m.msg.set_color.color.b = pixels[0].b;
    m.msg.set_color.has_color = true;

    static uint8_t buf[128];
    pb_ostream_t stream = pb_ostream_from_buffer(buf, sizeof(buf));
    if (pb_encode(&stream, si_sl_Msg_fields, &m) == 0) {
      LOG_ERR("failed to encode color message: %s", PB_GET_ERROR(&stream));
    } else {
      tincan_speak(buf, stream.bytes_written);
    }
  }
  return 0;
}

static int status_led_init(void) {
  if (!device_is_ready(strip)) {
    LOG_ERR("LED strip device is not ready");
    return -ENODEV;
  }

#if IS_ENABLED(CONFIG_ZMK_2NDIDEAL_STATUS_LED_TEST)
  status_led_test_walk();
#endif

  memset(pixels, 0, sizeof(pixels));
  led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);

  return 0;
}

ZMK_LISTENER(status_led, status_led_event_listener);
ZMK_SUBSCRIPTION(status_led, zmk_layer_state_changed);

#else // IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_PERIPHERAL)

#include <pb_decode.h>
#include <proto/si-sl.pb.h>

static void status_led_decode_cmds(const si_sl_Command *m) {
  for (size_t i = 0; i < m->cmds_count; i++) {
    switch (m->cmds[i]) {
    case si_sl_Commands_CMD_NOP:
      LOG_DBG("received NOP");
      break;
    case si_sl_Commands_CMD_TURN_ON:
      LOG_DBG("received TURN_ON");
      status_led_set_backlight(1);
      break;
    case si_sl_Commands_CMD_TURN_OFF:
      LOG_DBG("received TURN_OFF");
      status_led_set_backlight(0);
      break;
    default:
      LOG_ERR("received invalid command: %d", m->cmds[i]);
      break;
    }
  }
}

static void status_led_set_color(const si_sl_SetColor *sc) {
  if (sc->strip_index >= STRIP_NUM_PIXELS) {
    LOG_ERR("received invalid led index %u", sc->strip_index);
    return;
  };
  pixels[sc->strip_index].r = sc->color.r;
  pixels[sc->strip_index].g = sc->color.g;
  pixels[sc->strip_index].b = sc->color.b;
  LOG_DBG("set #%u to (%u, %u, %u)", pixels[sc->strip_index].r,
          pixels[sc->strip_index].g, pixels[sc->strip_index].b);
  led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
}

static void status_led_set_colors(const si_sl_SetColors *sc) {
  for (size_t i = 0; i < sc->colors_count; i++) {
    if (sc->colors[i].strip_index >= STRIP_NUM_PIXELS) {
      LOG_ERR("received invalid led index %u", sc->colors[i].strip_index);
      return;
    };
    pixels[sc->colors[i].strip_index].r = sc->colors[i].color.r;
    pixels[sc->colors[i].strip_index].g = sc->colors[i].color.g;
    pixels[sc->colors[i].strip_index].b = sc->colors[i].color.b;
    LOG_DBG("set #%u to (%u, %u, %u)", pixels[sc->colors[i].strip_index].r,
            pixels[sc->colors[i].strip_index].g,
            pixels[sc->colors[i].strip_index].b);
  }
  led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
}

static void status_led_set_all_colors(const si_sl_SetAllColors *sac) {
  for (size_t i = 0; i < sac->colors_count && i < STRIP_NUM_PIXELS; i++) {
    pixels[i].r = sac->colors[i].r;
    pixels[i].g = sac->colors[i].g;
    pixels[i].b = sac->colors[i].b;
    LOG_DBG("set #%u to (%u, %u, %u)", pixels[i].r, pixels[i].g, pixels[i].b);
  }
  led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
}

static void status_led_on_tincan(const void *payload, size_t length) {
  si_sl_Msg m = si_sl_Msg_init_default;
  pb_istream_t stream = pb_istream_from_buffer(payload, length);
  if (pb_decode(&stream, si_sl_Msg_fields, &m) == 0) {
    LOG_ERR("failed to decode message: %s", PB_GET_ERROR(&stream));
  } else {
    switch (m.which_msg) {
    case si_sl_Msg_cmd_tag:
      status_led_decode_cmds(&m.msg.cmd);
      break;
    case si_sl_Msg_set_color_tag:
      status_led_set_color(&m.msg.set_color);
      break;
    case si_sl_Msg_set_colors_tag:
      status_led_set_colors(&m.msg.set_colors);
      break;
    case si_sl_Msg_set_all_colors_tag:
      status_led_set_all_colors(&m.msg.set_all_colors);
      break;
    default:
      LOG_ERR("unknown msg subtype %d", m.which_msg);
      break;
    }
  }
}

static void status_led_peripheral_connected(struct bt_conn *conn, uint8_t err) {
  pixels[0] = colors[err ? COLOR_ERR : COLOR_OK];
  led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
  if (err) {
    LOG_WRN("connection to central has errors: 0x%02x", err);
  } else {
    tincan_listen(status_led_on_tincan);
  }
}

static void status_led_peripheral_disconnected(struct bt_conn *conn,
                                               uint8_t reason) {
  pixels[0] = colors[COLOR_ERR];
  led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
}

static int status_led_init(void) {
  if (!device_is_ready(strip)) {
    LOG_ERR("LED strip device is not ready");
    return -ENODEV;
  }

#if IS_ENABLED(CONFIG_ZMK_2NDIDEAL_STATUS_LED_TEST)
  status_led_test_walk();
#endif

  memset(pixels, 0, sizeof(pixels));
  led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
  return 0;
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = status_led_peripheral_connected,
    .disconnected = status_led_peripheral_disconnected,
};

#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_PERIPHERAL)

SYS_INIT(status_led_init, POST_KERNEL, 85);
