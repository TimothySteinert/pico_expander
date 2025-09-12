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

enum class AlarmIntegrationType : uint8_t {
  ALARMO = 0,
  OTHER = 1
};

class K1AlarmListener : public Component, public api::CustomAPIDevice {
 public:
  void set_alarm_entity(const std::string &e) { alarm_entity_ = e; }
  void set_alarm_type(uint8_t t) { alarm_type_ = static_cast<AlarmIntegrationType>(t); }
  void set_text_sensor(K1AlarmListenerTextSensor *ts);
  void register_ha_connection_sensor(binary_sensor::BinarySensor *bs) { ha_connection_sensor_ = bs; }

  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  // Configuration
  std::string alarm_entity_;
  AlarmIntegrationType alarm_type_{AlarmIntegrationType::ALARMO};

  // Sensors
  K1AlarmListenerTextSensor *main_sensor_{nullptr};
  binary_sensor::BinarySensor *ha_connection_sensor_{nullptr};

  // Connection/state
  bool subscription_started_{false};
  bool last_api_connected_{true};

  // Base mapped state (after unknown/unavailable mapping)
  std::string current_base_state_;
  std::string last_published_state_;

  // Attribute cache (Alarmo only)
  struct AttrCache {
    std::string arm_mode;
    std::string next_state;
    std::string bypassed_sensors;
    bool arm_mode_seen{false};
    bool next_state_seen{false};
    bool bypassed_seen{false};
  } attr_;

  bool attributes_supported_{false};
  bool waiting_for_attributes_{false};
  bool initial_state_published_{false};  // ensures we publish connection_timeout exactly once at boot

  // Constants
  static constexpr const char *CONNECTION_TIMEOUT_STATE = "connection_timeout";

  // HA callbacks
  void ha_state_callback_(std::string state);
  void arm_mode_attr_callback_(std::string value);
  void next_state_attr_callback_(std::string value);
  void bypassed_attr_callback_(std::string value);

  // Connection helpers
  bool api_connected_now_() const;
  void update_connection_sensor_(bool connected);
  void enforce_connection_state_();

  // Logic
  std::string map_alarm_state_(const std::string &raw) const;
  bool state_requires_attributes_(const std::string &mapped) const;
  bool have_required_attributes_for_state_(const std::string &mapped) const;
  std::string compute_inferred_state_(const std::string &mapped) const;
  void attempt_publish_alarmo_();  // publish when requirements satisfied
  bool is_truthy_attr_list_(const std::string &v) const;

  // Helpers
  static bool attr_has_value_(const std::string &v);
  static std::string lower_copy_(const std::string &s);
  static std::string trim_copy_(const std::string &s);

  // Publishing
  void publish_state_if_changed_(const std::string &st);
  void publish_initial_connection_timeout_();
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
    if (parent_ && type_ == K1AlarmListenerBinarySensorType::HA_CONNECTED)
      parent_->register_ha_connection_sensor(this);
  }
  void dump_config() override {}
 protected:
  K1AlarmListener *parent_{nullptr};
  K1AlarmListenerBinarySensorType type_{K1AlarmListenerBinarySensorType::HA_CONNECTED};
};

}  // namespace k1_alarm_listener
}  // namespace esphome
