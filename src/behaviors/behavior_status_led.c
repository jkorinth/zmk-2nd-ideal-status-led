#define DT_DRV_COMPAT zmk_behavior_status_led

#include <drivers/behavior.h>
#include <tincan/tincan.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/behavior.h>

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#ifdef CONFIG_ZMK_SPLIT_ROLE_CENTRAL
#include <pb.h>
#include <pb_encode.h>
#include <proto/si-sl.pb.h>
#include <status_led.h>

static int status_led_init(const struct device *dev) { return 0; }

static int
on_status_led_binding_pressed(struct zmk_behavior_binding *binding,
                              struct zmk_behavior_binding_event event) {
  LOG_DBG("%s", __func__);
  return ZMK_BEHAVIOR_OPAQUE;
}

static void setred(void) {
  si_sl_Msg m = si_sl_Msg_init_default;
  m.which_msg = si_sl_Msg_set_colors_tag;
  m.msg.set_colors.colors_count = 1;
  m.msg.set_colors.colors[0].strip_index = 0;
  m.msg.set_colors.colors[0].color.r = 255;
  m.msg.set_colors.colors[0].color.g = 10;
  m.msg.set_colors.colors[0].color.b = 10;
  m.msg.set_colors.colors[0].has_color = true;

  static uint8_t buf[128];
  pb_ostream_t stream = pb_ostream_from_buffer(buf, sizeof(buf));
  if (pb_encode(&stream, si_sl_Msg_fields, &m) == 0) {
    LOG_ERR("failed to encode message: %s", PB_GET_ERROR(&stream));
  } else {
    const size_t len = stream.bytes_written;
    LOG_DBG("sending %u bytes via tincan ...", len);
    tincan_speak(buf, len);
  }
}

static int
on_status_led_binding_released(struct zmk_behavior_binding *binding,
                               struct zmk_behavior_binding_event event) {
  LOG_DBG("%s", __func__);
  const uint32_t p = binding->param1;
  si_sl_Msg m = si_sl_Msg_init_default;
  m.which_msg = si_sl_Msg_cmd_tag;

  switch (p) {
  case SL_TURN_ON:
    LOG_DBG("received SL_TURN_ON");
    m.msg.cmd.cmds_count = 1;
    m.msg.cmd.cmds[0] = si_sl_Commands_CMD_TURN_ON;
    break;
  case SL_TURN_OFF:
    LOG_DBG("received SL_TURN_OFF");
    m.msg.cmd.cmds_count = 1;
    m.msg.cmd.cmds[0] = si_sl_Commands_CMD_TURN_OFF;
    setred();
    break;
  default:
    LOG_ERR("invalid parameter received: %u", p);
    return ZMK_BEHAVIOR_OPAQUE;
  };

  static uint8_t buf[128];
  pb_ostream_t stream = pb_ostream_from_buffer(buf, sizeof(buf));
  if (pb_encode(&stream, si_sl_Msg_fields, &m) == 0) {
    LOG_ERR("failed to encode message: %s", PB_GET_ERROR(&stream));
  } else {
    const size_t len = stream.bytes_written;
    LOG_DBG("sending %u bytes via tincan ...", len);
    tincan_speak(buf, len);
  }

  return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api status_led_driver_api = {
    .binding_pressed = on_status_led_binding_pressed,
    .binding_released = on_status_led_binding_released,
};

#define STATUS_LED_INST(n)                                                     \
  BEHAVIOR_DT_INST_DEFINE(n, status_led_init, NULL, NULL, NULL, POST_KERNEL,   \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                 \
                          &status_led_driver_api);

DT_INST_FOREACH_STATUS_OKAY(STATUS_LED_INST)

#endif
#endif
