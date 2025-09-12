#pragma once
#include <string>
#include <cstdint>

namespace esphome {
namespace k1_alarm_listener {

// Alarm integration types
enum class AlarmIntegrationType : uint8_t {
  ALARMO = 0,
  OTHER = 1
};

// Holds all internal state & logic for inferring the final alarm state
class K1AlarmStateHandling {
 public:
  void set_alarm_type(AlarmIntegrationType t) { alarm_type_ = t; }
  void set_timeouts(uint32_t incorrect_pin_ms, uint32_t failed_open_ms) {
    incorrect_pin_timeout_ms_ = incorrect_pin_ms;
    failed_open_sensors_timeout_ms_ = failed_open_ms;
  }

  // Base connection (true=connected) + raw HA state feed
  void set_connection(bool connected);
  void on_raw_state(const std::string &raw_state);         // entity state
  void on_attr_arm_mode(const std::string &v);             // arm_mode attr
  void on_attr_next_state(const std::string &v);           // next_state attr
  void on_attr_bypassed(const std::string &v);             // bypassed_sensors attr
  void on_failed_arm_reason(const std::string &reason);    // API service

  // Periodic tick for override expiry (now_ms in ms)
  void tick(uint32_t now_ms);

  // Query
  bool needs_publish() const { return publish_dirty_; }
  void clear_publish_flag() { publish_dirty_ = false; }

  // Effective state (already accounts for override & connection timeout)
  std::string current_effective_state(uint32_t now_ms);

  // Accessors for diagnostics
  bool override_active() const { return override_active_; }
  std::string override_state() const { return override_state_; }

  // Called if output layer wants to force a recompute (e.g. after override end)
  void force_recompute() { publish_dirty_ = true; }

 private:
  // ----- Configuration -----
  AlarmIntegrationType alarm_type_{AlarmIntegrationType::ALARMO};
  uint32_t incorrect_pin_timeout_ms_{2000};
  uint32_t failed_open_sensors_timeout_ms_{2000};

  // ----- Connection & raw state -----
  bool api_connected_{true};
  std::string base_state_mapped_;    // mapped raw (unavailable/unknown -> connection_timeout)

  // ----- Attributes cache -----
  struct AttrCache {
    std::string arm_mode;
    std::string next_state;
    std::string bypassed;
    bool arm_mode_seen{false};
    bool next_state_seen{false};
    bool bypassed_seen{false};
  } attr_;

  bool attributes_supported_{false};

  // Transitional waiting logic
  bool waiting_for_transitional_attrs_{false};
  uint8_t publish_retry_count_{0};
  static constexpr uint8_t MAX_PUBLISH_RETRIES = 3;

  // ----- Override (incorrect_pin / failed_open_sensors) -----
  bool override_active_{false};
  std::string override_state_;
  uint32_t override_end_ms_{0};

  // ----- Dirty flag for outward publish -----
  bool publish_dirty_{true};    // start true so initial placeholder is published

  // ----- Helpers / logic -----
  std::string map_raw_state_(const std::string &raw) const;
  bool is_transitional_(const std::string &mapped) const;
  bool have_transitional_attrs_(const std::string &mapped) const;
  bool attr_has_value_(const std::string &v) const;
  bool is_truthy_list_(const std::string &v) const;
  std::string infer_state_core_(const std::string &mapped) const;
  std::string compute_effective_without_override_() const;

  void mark_dirty_() { publish_dirty_ = true; }

  void start_override_(const std::string &st, uint32_t dur_ms, uint32_t now_ms);
  void clear_override_();
};

}  // namespace k1_alarm_listener
}  // namespace esphome
