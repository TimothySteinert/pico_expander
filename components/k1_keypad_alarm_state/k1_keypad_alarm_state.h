#pragma once
#include "esphome/core/component.h"
#include "esphome/components/text_sensor/text_sensor.h"

namespace esphome {
namespace k1_keypad_alarm_state {

class K1KeypadAlarmState : public Component, public text_sensor::TextSensor {
 public:
  void set_entity_id(const std::string &entity_id) { this->entity_id_ = entity_id; }
  void set_name(const std::string &name) { this->name_ = name; }

  void setup() override;
  void dump_config() override;
  void loop() override;

  void process_state(const std::string &state);

 protected:
  std::string entity_id_;
  std::string name_;
};

}  // namespace k1_keypad_alarm_state
}  // namespace esphome
