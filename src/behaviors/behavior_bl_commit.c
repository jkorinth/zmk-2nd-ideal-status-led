#define DT_DRV_COMPAT zmk_behavior_bl_commit

#include <drivers/behavior.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/behavior.h>

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#ifdef CONFIG_ZMK_SPLIT_ROLE_CENTRAL
#include <bl_ids.h>
#include <bl_store.h>

static int bl_commit_init(const struct device *dev) { return 0; }

static int on_bl_commit_binding_pressed(struct zmk_behavior_binding *binding,
                                        struct zmk_behavior_binding_event event) {
  return ZMK_BEHAVIOR_OPAQUE;
}

static int on_bl_commit_binding_released(struct zmk_behavior_binding *binding,
                                         struct zmk_behavior_binding_event event) {
  switch (binding->param1) {
  case BL_COMMIT:
    bl_store_commit();
    break;
  case BL_DISCARD:
    bl_store_discard();
    break;
  default:
    LOG_ERR("invalid parameter received: %u", binding->param1);
    break;
  }
  return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api bl_commit_driver_api = {
    .binding_pressed = on_bl_commit_binding_pressed,
    .binding_released = on_bl_commit_binding_released,
};

#define BL_COMMIT_INST(n)                                                                         \
  BEHAVIOR_DT_INST_DEFINE(n, bl_commit_init, NULL, NULL, NULL, POST_KERNEL,                        \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &bl_commit_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BL_COMMIT_INST)

#endif
#endif
