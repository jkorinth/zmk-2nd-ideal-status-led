#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/logging/log.h>

#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/keymap.h>

LOG_MODULE_REGISTER(status_led, CONFIG_ZMK_LOG_LEVEL);

#define STRIP_NODE 		DT_CHOSEN(zmk_underglow)
#define STRIP_NUM_PIXELS 	DT_PROP(STRIP_NODE, chain_length)

static const struct device *const strp = DEVICE_DT_GET(STRIP_NODE);
static struct led_rgb pixels[STRIP_NUM_PIXELS];

static uint8_t current_layer;
static struct k_work_delayable update_work;

static void update_work_handler(struct k_work *work) {
	LOG_DBG("update runs!");
	k_work_schedule(&update_work, K_MSEC(CONFIG_ZMK_2NDIDEAL_STATUS_LED_UPDATE_INTERVAL_MS));
}

static int status_led_event_listener(const zmk_event_t *eh) {
	current_layer = zmk_keymap_highest_layer_active();
	LOG_DBG("layer: %d", current_layer);
}

ZMK_LISTENER(status_led, status_led_event_listener);
ZMK_SUBSCRIPTION(status_led, zmk_layer_state_changed);

static int status_led_init(void)
{
	if (!device_is_ready(strip)) {
		LOG_ERR("LED strip device is not ready");
		return -ENODEV;
	}

	k_work_init_delayable(&update_work, update_work_handler);
	current_layer = zmk_keymap_highest_layer_active();
	k_work_schedule(&update_work, K_NO_WAIT);

	return 0;
}

SYS_INIT(status_led_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
