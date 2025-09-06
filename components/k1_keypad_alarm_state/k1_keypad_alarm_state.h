#pragma once
#include "esphome/core/component.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/api/custom_api_device.h"

namespace esphome {
namespace k1_keypad_alarm_state {

class K1KeypadAlarmState : public Component,
                           public text_sensor::TextSensor,
                           public api::CustomAPIDevice {
 public:
  void set_entity_id(const std::string &entity_id) { entity_id_ = entity_id; }

  void setup() override;
  void dump_config() override;

 protected:
  void on_ha_state_(std::string state);
  std::string entity_id_;
};

}  // namespace k1_keypad_alarm_state
}  // namespace esphome
