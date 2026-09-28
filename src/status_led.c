#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <tincan/tincan.h>
#include <zmk/event_manager.h>

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
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
  COLOR_BACKGROUND,
  COLOR_COUNT
} status_led_color_t;

static struct led_rgb colors[COLOR_COUNT] = {
    {.r = 0, .g = 0, .b = 0},
    {.r = STRIP_BRIGHTNESS, .g = STRIP_BRIGHTNESS, .b = STRIP_BRIGHTNESS},
    {.r = 0, .g = 0, .b = STRIP_BRIGHTNESS},
    {.r = STRIP_BRIGHTNESS, .g = STRIP_BRIGHTNESS, .b = STRIP_BRIGHTNESS},
};

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
#endif // IS_ENABLED(CONFIG_ZMK_2NDIDEAL_STATUS_LED_TEST)

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
static uint8_t current_layer;

static int status_led_event_listener(const zmk_event_t *eh) {
  uint8_t prev_layer = current_layer;
  current_layer = zmk_keymap_highest_layer_active();
  LOG_DBG("prev layer: %d, curr layer: %d", prev_layer, current_layer);
  if (prev_layer != current_layer) {
    pixels[0] = colors[current_layer > sizeof(colors) / sizeof(*colors)
                           ? 0
                           : current_layer];
    led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
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
  for (size_t i = 1; i < STRIP_NUM_PIXELS; i++) {
    pixels[i] = colors[COLOR_BACKGROUND];
  }
  led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);

  return 0;
}

ZMK_LISTENER(status_led, status_led_event_listener);
ZMK_SUBSCRIPTION(status_led, zmk_layer_state_changed);

#else // IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_PERIPHERAL)

#include <pb_decode.h>
#include <proto/si-sl.pb.h>

static void status_led_turn_on(void) {
  memset(pixels, 0, sizeof(pixels));
  pixels[0] = colors[COLOR_ERR];
  led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
}

static void status_led_turn_off(void) {
  memset(pixels, 0, sizeof(pixels));
  led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
}

static void status_led_decode_cmds(const si_sl_Command *m) {
  for (size_t i = 0; i < m->cmds_count; i++) {
    switch (m->cmds[i]) {
    case si_sl_Commands_CMD_NOP:
      LOG_DBG("received NOP");
      break;
    case si_sl_Commands_CMD_TURN_ON:
      LOG_DBG("received TURN_ON");
      status_led_turn_on();
      break;
    case si_sl_Commands_CMD_TURN_OFF:
      LOG_DBG("received TURN_OFF");
      status_led_turn_off();
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

  status_led_turn_off();

  for (size_t i = 1; i < STRIP_NUM_PIXELS; i++) {
    pixels[i] = colors[COLOR_BACKGROUND];
  }
  led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
  return 0;
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = status_led_peripheral_connected,
    .disconnected = status_led_peripheral_disconnected,
};

#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_PERIPHERAL)

SYS_INIT(status_led_init, POST_KERNEL, 85);
