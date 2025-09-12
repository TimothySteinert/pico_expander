#pragma once
#include "esphome/core/component.h"
#include "esphome/components/api/custom_api_device.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

#include "state_handling.h"
#include "api.h"
#include "state_main.h"

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
  void set_alarm_type(uint8_t t) { handling_.set_alarm_type(static_cast<AlarmIntegrationType>(t)); }
  void set_incorrect_pin_timeout_ms(uint32_t v) { handling_.set_timeouts(v, failed_open_sensors_timeout_ms_); incorrect_pin_timeout_ms_ = v; }
  void set_failed_open_sensors_timeout_ms(uint32_t v) { handling_.set_timeouts(incorrect_pin_timeout_ms_, v); failed_open_sensors_timeout_ms_ = v; }

  // Sensor registration
  void set_text_sensor(K1AlarmListenerTextSensor *ts) { state_main_.attach_text_sensor(ts); }
  void register_ha_connection_sensor(binary_sensor::BinarySensor *bs);

  // Lifecycle
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  // Called by API layer when new data arrives
  void schedule_state_publish() { state_main_.schedule_publish(); }
  void set_timeout(uint32_t ms, std::function<void()> &&f) { Component::set_timeout(ms, std::move(f)); }

 protected:
  // Config
  std::string alarm_entity_;
  uint32_t incorrect_pin_timeout_ms_{2000};
  uint32_t failed_open_sensors_timeout_ms_{2000};

  // Modules
  K1AlarmStateHandling handling_;
  K1AlarmApi api_;
  K1AlarmStateMain state_main_;

  // Connection sensor pointer
  binary_sensor::BinarySensor *ha_connection_sensor_{nullptr};
  bool last_api_connected_{true};
  bool subscription_started_{false};
  bool initial_placeholder_done_{false};

  // Internal helpers
  bool api_connected_now_() const;
  void update_connection_sensor_(bool connected);

  // HA callbacks (forward to api_)
  void ha_state_callback_(std::string s);
  void arm_mode_attr_callback_(std::string v);
  void next_state_attr_callback_(std::string v);
  void bypassed_attr_callback_(std::string v);
  void failed_arm_service_(std::string reason);
};

// Text sensor
class K1AlarmListenerTextSensor : public text_sensor::TextSensor, public Component {
 public:
  void set_parent(K1AlarmListener *p) { parent_ = p; }
  void setup() override {
    if (parent_) parent_->set_text_sensor(this);
  }
  void dump_config() override {}
 private:
  K1AlarmListener *parent_{nullptr};
};

// Binary sensor
class K1AlarmListenerBinarySensor : public binary_sensor::BinarySensor, public Component {
 public:
  void set_parent(K1AlarmListener *p) { parent_ = p; }
  void set_type(uint8_t t) { type_ = static_cast<K1AlarmListenerBinarySensorType>(t); }
  void setup() override {
    if (parent_ && type_ == K1AlarmListenerBinarySensorType::HA_CONNECTED)
      parent_->register_ha_connection_sensor(this);
  }
  void dump_config() override {}
 private:
  K1AlarmListener *parent_{nullptr};
  K1AlarmListenerBinarySensorType type_{K1AlarmListenerBinarySensorType::HA_CONNECTED};
};

}  // namespace k1_alarm_listener
}  // namespace esphome
