#pragma once

#include "esphome/core/component.h"
#include "esphome/components/script/script.h"
#include <string>
#include <vector>
#include <cstdint>

namespace esphome {
namespace k1_arm_handler {

// We know (by YAML) the script parameters are all strings:
// mode, pin, force, skip, prefix
using ConcreteScript = script::Script<
    std::string, std::string, std::string, std::string, std::string>;

class K1ArmHandlerComponent : public Component {
 public:
  void set_alarm_entity_id(const std::string &v) { alarm_entity_id_ = v; }
  void set_force_prefix(const std::string &v) { force_prefix_ = v; }
  void set_skip_delay_prefix(const std::string &v) { skip_delay_prefix_ = v; }
  // Bridge setter (receives raw pointer from Python codegen)
  void set_callback_script_ptr(void *p) { script_ptr_ = p; }

  void setup() override {}
  void loop() override {}

  void handle_a0_message(const std::vector<uint8_t> &bytes);
  void execute_command(const std::string &prefix,
                       const std::string &arm_select,
                       const std::string &pin);

 protected:
  std::string alarm_entity_id_;
  std::string force_prefix_{"999"};
  std::string skip_delay_prefix_{"998"};

  void *script_ptr_{nullptr};  // stored as void*, cast on use

  std::string map_digit_(uint8_t code) const;
  const char * map_arm_select_(uint8_t code) const;

  void invoke_script_(const std::string &mode,
                      const std::string &pin,
                      bool force_flag,
                      bool skip_delay_flag,
                      const std::string &prefix);
};

}  // namespace k1_arm_handler
}  // namespace esphome
