#pragma once
#include "esphome/core/component.h"
#include "esphome/components/api/custom_api_device.h"
#include "esphome/components/select/select.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/core/preferences.h"
#include <vector>
#include <string>

namespace esphome {
namespace k1_ready {

class K1ReadySelect;
class K1ReadyBinarySensor;

// Engine holding all readiness state and the current selected mode
class K1Ready : public Component, public api::CustomAPIDevice {
 public:
  // Config setters (from codegen)
  void add_option(const std::string &opt) { options_.push_back(opt); }
  void set_initial_option(const std::string &opt) { initial_option_ = opt; }
  void set_optimistic(bool v) { optimistic_ = v; }
  void set_restore_value(bool v) { restore_value_ = v; }

  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  // API service registration
  void register_ready_update_service();

  // Called by facade select when user chooses a mode
  void user_select(const std::string &mode);

  // Facade registrations
  void register_select(K1ReadySelect *sel) { select_ = sel; push_select_state_(); }
  void register_ready_binary_sensor(K1ReadyBinarySensor *bs) { ready_sensor_ = bs; push_ready_state_(); }

  // Query
  bool ready_for_mode(const std::string &mode) const;
  bool current_mode_ready() const { return ready_for_mode(current_mode_); }
  const std::vector<std::string> &options() const { return options_; }
  const std::string &current_mode() const { return current_mode_; }

 protected:
  // Service handler
  void api_ready_update_(bool armed_away,
                         bool armed_home,
                         bool armed_night,
                         bool armed_custom_bypass,
                         bool armed_vacation);

  // Internal helpers
  void load_or_init_selection_();
  void save_selection_();
  void push_select_state_();
  void push_ready_state_();

  // Config/state
  std::vector<std::string> options_;
  std::string initial_option_;
  bool optimistic_{true};
  bool restore_value_{true};

  // Current selection
  std::string current_mode_;
  ESPPreferenceObject pref_;
  bool preference_inited_{false};

  // Readiness flags
  bool ready_away_{false};
  bool ready_home_{false};
  bool ready_night_{false};
  bool ready_custom_{false};
  bool ready_vacation_{false};

  // Attached facades
  K1ReadySelect *select_{nullptr};
  K1ReadyBinarySensor *ready_sensor_{nullptr};
};

// Facade select (no business logic)
class K1ReadySelect : public select::Select, public Component {
 public:
  explicit K1ReadySelect(K1Ready *engine) : engine_(engine) {}
  void bind_to_engine() { if (engine_) engine_->register_select(this); }
  void setup() override {}
  void dump_config() override {}
  select::SelectTraits get_traits() override;
 protected:
  void control(const std::string &value) override;
  K1Ready *engine_;
};

// Binary sensor representing readiness for currently selected mode
class K1ReadyBinarySensor : public binary_sensor::BinarySensor, public Component {
 public:
  explicit K1ReadyBinarySensor(K1Ready *engine) : engine_(engine) {}
  void setup() override {}
  void dump_config() override {}
  void update_from_engine(bool val) { this->publish_state(val); }
 protected:
  K1Ready *engine_;
};

}  // namespace k1_ready
}  // namespace esphome
