#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/logging/log.h>

#include <zmk/event_manager.h>

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#include <zmk/events/layer_state_changed.h>
#else // !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#include <zephyr/bluetooth/conn.h>
#endif // !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#include <zmk/keymap.h>

LOG_MODULE_REGISTER(status_led, CONFIG_ZMK_LOG_LEVEL);

#define STRIP_NODE 		DT_CHOSEN(zmk_underglow)
#define STRIP_NUM_PIXELS 	DT_PROP(STRIP_NODE, chain_length)
#define STRIP_BRIGHTNESS 	CONFIG_ZMK_2NDIDEAL_STATUS_LED_BRIGHTNESS

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
	{ .r = 0,                .g = 0,                 .b = 0 },
	{ .r = STRIP_BRIGHTNESS, .g = STRIP_BRIGHTNESS , .b = STRIP_BRIGHTNESS },
	{ .r = 0,                .g = 0,                 .b = STRIP_BRIGHTNESS },
	{ .r = STRIP_BRIGHTNESS, .g = STRIP_BRIGHTNESS , .b = STRIP_BRIGHTNESS },
};

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
static uint8_t current_layer;

static int status_led_event_listener(const zmk_event_t *eh) {
	uint8_t prev_layer = current_layer;
	current_layer = zmk_keymap_highest_layer_active();
	LOG_DBG("prev layer: %d, curr layer: %d", prev_layer, current_layer);
	if (prev_layer != current_layer) {
		pixels[0] = colors[current_layer > sizeof(colors)/sizeof(*colors) ? 0 : current_layer];
		led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
	}
	return 0;
}

static int status_led_init(void)
{
	if (!device_is_ready(strip)) {
		LOG_ERR("LED strip device is not ready");
		return -ENODEV;
	}

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

static void status_led_peripheral_connected(struct bt_conn *conn, uint8_t err)
{
	pixels[0] = colors[err ? COLOR_ERR : COLOR_OK];
	led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
	if (err) {
		LOG_WRN("connection to central has errors: 0x%02x", err);
	}
}

static void status_led_peripheral_disconnected(struct bt_conn *conn, uint8_t reason)
{
	pixels[0] = colors[COLOR_ERR];
	led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
}

static int status_led_init(void)
{
	if (!device_is_ready(strip)) {
		LOG_ERR("LED strip device is not ready");
		return -ENODEV;
	}

	memset(pixels, 0, sizeof(pixels));
	pixels[0] = colors[COLOR_ERR];
	led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
	return 0;
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = status_led_peripheral_connected,
	.disconnected = status_led_peripheral_disconnected,
};

#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_PERIPHERAL)

SYS_INIT(status_led_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
