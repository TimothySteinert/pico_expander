#pragma once

#include "esphome/core/component.h"
#include "esphome/components/api/custom_api_device.h"
#include <vector>
#include <string>
#include <cstdint>

namespace esphome {
namespace k1_arm_handler {

class K1ArmHandlerComponent : public Component, public api::CustomAPIDevice {
 public:
  void set_alarm_entity_id(const std::string &v) { alarm_entity_id_ = v; }
  void set_arm_service(const std::string &v) { arm_service_ = v; }
  void set_disarm_service(const std::string &v) { disarm_service_ = v; }
  void set_force_prefix(const std::string &v) { force_prefix_ = v; }
  void set_skip_delay_prefix(const std::string &v) { skip_delay_prefix_ = v; }

  // Feed already-parsed strings
  void execute_command(const std::string &prefix,
                       const std::string &arm_select,
                       const std::string &pin);

  // Parse a full raw A0 message frame (expects first byte 0xA0, length >= 21)
  void handle_a0_message(const std::vector<uint8_t> &bytes);

  void setup() override {}
  void loop() override {}

 protected:
  std::string alarm_entity_id_;
  std::string arm_service_{"alarmo.arm"};
  std::string disarm_service_{"alarm_control_panel.alarm_disarm"};
  std::string force_prefix_{"999"};
  std::string skip_delay_prefix_{"998"};

  std::string map_digit_(uint8_t code) const;
  const char * map_arm_select_(uint8_t code) const;
  void call_disarm_(const std::string &pin);
  void call_arm_(const std::string &mode, const std::string &pin,
                 bool force_flag, bool skip_delay_flag);
};

}  // namespace k1_arm_handler
}  // namespace esphome
