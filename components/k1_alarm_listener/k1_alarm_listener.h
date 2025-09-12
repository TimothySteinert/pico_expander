#pragma once

#include "esphome/core/component.h"
#include "esphome/components/api/custom_api_device.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

#ifdef USE_API
#include "esphome/components/api/api_server.h"
#endif

namespace esphome {
namespace k1_alarm_listener {

class K1AlarmListenerTextSensor;
class K1AlarmListenerBinarySensor;

enum class K1AlarmListenerBinarySensorType : uint8_t {
  HA_CONNECTED = 0
};

class K1AlarmListener : public Component, public api::CustomAPIDevice {
 public:
  void set_alarm_entity(const std::string &e) { alarm_entity_ = e; }
  void set_text_sensor(K1AlarmListenerTextSensor *ts) { state_sensor_ = ts; }
  void register_ha_connection_sensor(binary_sensor::BinarySensor *bs) { ha_connection_sensor_ = bs; }

  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  std::string alarm_entity_;
  K1AlarmListenerTextSensor *state_sensor_{nullptr};
  binary_sensor::BinarySensor *ha_connection_sensor_{nullptr};

  bool subscription_started_{false};
  bool last_api_connected_{true};
  std::string last_published_state_;

  // Callbacks
  void ha_state_callback_(std::string state);

  // Helpers
  bool api_connected_now_() const;
  void update_connection_sensor_(bool connected);
  void enforce_connection_state_();
  std::string map_alarm_state_(const std::string &raw) const;
  bool is_known_alarm_state_(const std::string &s) const;
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

class K1AlarmListenerBinarySensor : public binary_sensor::BinarySensor, public Component {
 public:
  void set_parent(K1AlarmListener *p) { parent_ = p; }
  void set_type(uint8_t t) { type_ = static_cast<K1AlarmListenerBinarySensorType>(t); }
  void setup() override {
    if (parent_ && type_ == K1AlarmListenerBinarySensorType::HA_CONNECTED) {
      parent_->register_ha_connection_sensor(this);
      // Initial publish deferred until parent loop() runs
    }
  }
  void dump_config() override {}

 protected:
  K1AlarmListener *parent_{nullptr};
  K1AlarmListenerBinarySensorType type_{K1AlarmListenerBinarySensorType::HA_CONNECTED};
};

}  // namespace k1_alarm_listener
}  // namespace esphome
