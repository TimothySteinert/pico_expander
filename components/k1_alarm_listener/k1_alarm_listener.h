#pragma once

#include "esphome/core/component.h"
#include "esphome/components/api/custom_api_device.h"
#include "esphome/components/text_sensor/text_sensor.h"

namespace esphome {
namespace k1_alarm_listener {

class K1AlarmListenerTextSensor;  // forward

class K1AlarmListener : public Component, public api::CustomAPIDevice {
 public:
  void set_alarm_entity(const std::string &e) { alarm_entity_ = e; }
  void set_text_sensor(K1AlarmListenerTextSensor *ts) { state_sensor_ = ts; }

  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  std::string alarm_entity_;
  K1AlarmListenerTextSensor *state_sensor_{nullptr};
  bool subscription_started_{false};
};

class K1AlarmListenerTextSensor : public text_sensor::TextSensor, public Component {
 public:
  void set_parent(K1AlarmListener *p) { parent_ = p; }
  void setup() override {
    if (parent_) parent_->set_text_sensor(this);
  }
  void dump_config() override {}

 protected:
  K1AlarmListener *parent_{nullptr};
};

}  // namespace k1_alarm_listener
}  // namespace esphome
