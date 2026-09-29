#define DT_DRV_COMPAT zmk_behavior_bl_select

#include <drivers/behavior.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/behavior.h>

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#ifdef CONFIG_ZMK_SPLIT_ROLE_CENTRAL
#include <bl_ids.h>
#include <bl_store.h>

static int bl_select_init(const struct device *dev) { return 0; }

static int on_bl_select_binding_pressed(struct zmk_behavior_binding *binding,
                                        struct zmk_behavior_binding_event event) {
  return ZMK_BEHAVIOR_OPAQUE;
}

static int on_bl_select_binding_released(struct zmk_behavior_binding *binding,
                                         struct zmk_behavior_binding_event event) {
  const uint32_t p = binding->param1;
  struct bl_led_ref led = {
      .peripheral = (p & BL_HALF_PERIPHERAL) != 0,
      .chain_idx = p & 0x0F,
  };
  if (led.chain_idx < 1 || led.chain_idx > 6) {
    LOG_ERR("invalid parameter received: %u", p);
    return ZMK_BEHAVIOR_OPAQUE;
  }
  bl_store_select(led);
  return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api bl_select_driver_api = {
    .binding_pressed = on_bl_select_binding_pressed,
    .binding_released = on_bl_select_binding_released,
};

#define BL_SELECT_INST(n)                                                                        \
  BEHAVIOR_DT_INST_DEFINE(n, bl_select_init, NULL, NULL, NULL, POST_KERNEL,                       \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &bl_select_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BL_SELECT_INST)

#endif
#endif
