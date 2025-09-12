#pragma once
#include "esphome/core/component.h"
#include "esphome/components/select/select.h"
#include "esphome/core/preferences.h"
#include "esphome/components/api/custom_api_device.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include <vector>
#include <string>

namespace esphome {
namespace k1_ready {

class K1ReadyBinarySensor;  // forward

class K1ReadySelect : public select::Select, public Component, public api::CustomAPIDevice {
 public:
  void add_option(const std::string &opt) { options_.push_back(opt); }
  void set_initial_option(const std::string &opt) { initial_option_ = opt; }
  void set_optimistic(bool v) { optimistic_ = v; }
  void set_restore_value(bool v) { restore_value_ = v; }

  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  // Called from Python code-gen to register API service
  void register_ready_update_service();

  // Binary sensor registration
  void register_ready_binary_sensor(K1ReadyBinarySensor *bs) { ready_binary_sensor_ = bs; publish_ready_state_(); }

  // Exposed so binary sensor (if needed) can query current readiness
  bool current_mode_ready() const;

 protected:
  // select::Select overrides
  void control(const std::string &value) override;
  select::SelectTraits get_traits() override;

  void publish_initial_state_();
  void save_();

  // API service handler
  void api_ready_update_(bool armed_away, bool armed_home, bool armed_night,
                         bool armed_custom_bypass, bool armed_vacation);

  // Internal readiness mapping & publishing
  void publish_ready_state_();
  bool ready_for_mode_(const std::string &mode) const;

  std::vector<std::string> options_;
  std::string initial_option_;
  bool optimistic_{true};
  bool restore_value_{true};

  ESPPreferenceObject pref_;
  bool has_restored_{false};
  std::string restored_value_;

  // Readiness flags
  bool ready_away_{false};
  bool ready_home_{false};
  bool ready_night_{false};
  bool ready_custom_{false};
  bool ready_vacation_{false};

  K1ReadyBinarySensor *ready_binary_sensor_{nullptr};
};

class K1ReadyBinarySensor : public binary_sensor::BinarySensor, public Component {
 public:
  explicit K1ReadyBinarySensor(K1ReadySelect *parent) : parent_(parent) {}
  void dump_config() override {}
  void setup() override {
    // Parent will push initial state after registration; nothing else required here
  }

  // Parent calls this when selection or readiness flags change
  void update_from_parent(bool value) { this->publish_state(value); }

 protected:
  K1ReadySelect *parent_{nullptr};
};

}  // namespace k1_ready
}  // namespace esphome
