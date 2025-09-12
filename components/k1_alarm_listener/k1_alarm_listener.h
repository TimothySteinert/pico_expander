#pragma once
#include "esphome/core/component.h"
#include "esphome/components/api/custom_api_device.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/core/log.h"

#ifdef USE_API
#include "esphome/components/api/api_server.h"
#endif

#include <string>
#include <cstdint>

namespace esphome {
namespace k1_alarm_listener {

enum class AlarmIntegrationType : uint8_t {
  ALARMO = 0,
  OTHER = 1
};

enum class K1AlarmListenerBinarySensorType : uint8_t {
  HA_CONNECTED = 0
};

class K1AlarmListenerTextSensor;
class K1AlarmListenerBinarySensor;

class K1AlarmListener : public Component, public api::CustomAPIDevice {
 public:
  // Configuration setters
  void set_alarm_entity(const std::string &e) { alarm_entity_ = e; }
  void set_alarm_type(uint8_t t) { alarm_type_ = static_cast<AlarmIntegrationType>(t); }
  void set_incorrect_pin_timeout_ms(uint32_t v) { incorrect_pin_timeout_ms_ = v; }
  void set_failed_open_sensors_timeout_ms(uint32_t v) { failed_open_sensors_timeout_ms_ = v; }

  // Sensors
  void set_text_sensor(K1AlarmListenerTextSensor *ts) { text_sensor_ = ts; if (!last_published_state_.empty()) publish_state_(last_published_state_); }
  void register_ha_connection_sensor(binary_sensor::BinarySensor *bs) {
    ha_connection_sensor_ = bs;
    update_connection_sensor_(api_connected_now_());
  }

  // Core lifecycle
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  // Called by internal code to schedule deferred publishing
  void schedule_state_publish();

  // Exposed so internal lambdas can use Component’s timeout
  void set_timeout(uint32_t ms, std::function<void()> &&f) { Component::set_timeout(ms, std::move(f)); }

 protected:
  // ----- Configuration -----
  std::string alarm_entity_;
  AlarmIntegrationType alarm_type_{AlarmIntegrationType::ALARMO};
  uint32_t incorrect_pin_timeout_ms_{2000};
  uint32_t failed_open_sensors_timeout_ms_{2000};

  // ----- Sensors -----
  K1AlarmListenerTextSensor *text_sensor_{nullptr};
  binary_sensor::BinarySensor *ha_connection_sensor_{nullptr};

  // ----- Connection state -----
  bool last_api_connected_{true};

  // ----- Raw & attribute state -----
  std::string raw_state_;              // raw HA state
  std::string mapped_state_;           // mapped base (unavailable->connection_timeout)
  std::string attr_arm_mode_;
  std::string attr_next_state_;
  std::string attr_bypassed_;

  bool attr_arm_mode_seen_{false};
  bool attr_next_state_seen_{false};
  bool attr_bypassed_seen_{false};
  bool attributes_supported_{false};

  // ----- Override handling -----
  bool override_active_{false};
  std::string override_state_;
  uint32_t override_end_ms_{0};

  // ----- Publish management -----
  bool publish_scheduled_{false};
  bool publish_pending_{false};
  std::string last_published_state_;
  bool initial_placeholder_done_{false};

  // Timing helpers
  static inline uint32_t now_ms_();

  // Internal helpers
  bool api_connected_now_() const;
  void update_connection_sensor_(bool connected);
  void publish_state_(const std::string &state);
  void publish_initial_placeholder_();
  void finalize_publish_();
  std::string compute_effective_state_();
  std::string infer_alarm_state_(const std::string &mapped);
  std::string map_raw_state_(const std::string &raw) const;
  bool is_transitional_generic_(const std::string &s) const;
  bool transitional_has_required_attrs_(const std::string &mapped) const;
  bool attr_has_value_(const std::string &v) const;
  bool is_truthy_list_(const std::string &v) const;

  void start_override_(const std::string &state, uint32_t dur_ms);
  void clear_override_();
  void handle_failed_arm_reason_(const std::string &reason);
  void tick_override_expiry_();

  // HA callbacks
  void ha_state_callback_(std::string s);
  void arm_mode_attr_callback_(std::string v);
  void next_state_attr_callback_(std::string v);
  void bypassed_attr_callback_(std::string v);
  void failed_arm_service_(std::string reason);
};

// Text sensor class
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

// Binary sensor class
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
