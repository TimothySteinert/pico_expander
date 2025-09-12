#pragma once

#include "esphome/core/component.h"
#include <vector>
#include <string>
#include <cstdint>

namespace esphome {
namespace k1_arm_handler {

class K1ArmHandlerComponent : public Component {
 public:
  void set_alarm_entity_id(const std::string &v) { alarm_entity_id_ = v; }
  void set_arm_service(const std::string &v) { arm_service_ = v; }
  void set_disarm_service(const std::string &v) { disarm_service_ = v; }
  void set_force_prefix(const std::string &v) { force_prefix_ = v; }
  void set_skip_delay_prefix(const std::string &v) { skip_delay_prefix_ = v; }

  void setup() override {}
  void loop() override {}

  // Public API
  void execute_command(const std::string &prefix,
                       const std::string &arm_select,
                       const std::string &pin);
  void handle_a0_message(const std::vector<uint8_t> &bytes);

 protected:
  // Config
  std::string alarm_entity_id_;
  std::string arm_service_{"alarmo.arm"};
  std::string disarm_service_{"alarm_control_panel.alarm_disarm"};
  std::string force_prefix_{"999"};
  std::string skip_delay_prefix_{"998"};

  // Helpers
  std::string map_digit_(uint8_t code) const;
  const char * map_arm_select_(uint8_t code) const;

  // Internal executors
  void call_disarm_(const std::string &pin);
  void call_arm_(const std::string &mode, const std::string &pin,
                 bool force_flag, bool skip_delay_flag);
};

}  // namespace k1_arm_handler
}  // namespace esphome
