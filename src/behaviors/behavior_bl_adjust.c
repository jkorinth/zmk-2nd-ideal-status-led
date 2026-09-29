#define DT_DRV_COMPAT zmk_behavior_bl_adjust

#include <drivers/behavior.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/behavior.h>

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#ifdef CONFIG_ZMK_SPLIT_ROLE_CENTRAL
#include <bl_store.h>

static int bl_adjust_init(const struct device *dev) { return 0; }

static int on_bl_adjust_binding_pressed(struct zmk_behavior_binding *binding,
                                        struct zmk_behavior_binding_event event) {
  return ZMK_BEHAVIOR_OPAQUE;
}

static int on_bl_adjust_binding_released(struct zmk_behavior_binding *binding,
                                         struct zmk_behavior_binding_event event) {
  bl_store_adjust((uint8_t)binding->param1, (int8_t)binding->param2);
  return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api bl_adjust_driver_api = {
    .binding_pressed = on_bl_adjust_binding_pressed,
    .binding_released = on_bl_adjust_binding_released,
};

#define BL_ADJUST_INST(n)                                                                         \
  BEHAVIOR_DT_INST_DEFINE(n, bl_adjust_init, NULL, NULL, NULL, POST_KERNEL,                        \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &bl_adjust_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BL_ADJUST_INST)

#endif
#endif
