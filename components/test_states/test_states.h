#pragma once
#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/core/log.h"
#include <vector>
#include <string>
#include <algorithm>

namespace esphome {
namespace test_states {

class ModeTrigger : public Trigger<> {
 public:
  ModeTrigger() = default;
};

class PreModeTrigger : public Trigger<> {
 public:
  PreModeTrigger() = default;
};

class TestStatesComponent : public Component {
 public:
  void setup() override {}
  void loop() override {}
  void dump_config() override;

  void set_mode_by_name(const std::string &name);
  const std::string &current_mode() const { return current_mode_; }

  // Intermode (pre-transition) triggers
  void add_inter_mode_trigger(PreModeTrigger *t) { intermode_trigs_.push_back(t); }

  // Add trigger methods for each fixed mode
  void add_disarmed_trigger(ModeTrigger *t) { disarmed_.push_back(t); }
  void add_triggered_trigger(ModeTrigger *t) { triggered_.push_back(t); }
  void add_connection_timeout_trigger(ModeTrigger *t) { connection_timeout_.push_back(t); }
  void add_incorrect_pin_trigger(ModeTrigger *t) { incorrect_pin_.push_back(t); }
  void add_failed_open_sensors_trigger(ModeTrigger *t) { failed_open_sensors_.push_back(t); }

  void add_arming_trigger(ModeTrigger *t) { arming_.push_back(t); }
  void add_arming_home_trigger(ModeTrigger *t) { arming_home_.push_back(t); }
  void add_arming_away_trigger(ModeTrigger *t) { arming_away_.push_back(t); }
  void add_arming_night_trigger(ModeTrigger *t) { arming_night_.push_back(t); }
  void add_arming_vacation_trigger(ModeTrigger *t) { arming_vacation_.push_back(t); }
  void add_arming_custom_bypass_trigger(ModeTrigger *t) { arming_custom_bypass_.push_back(t); }

  void add_pending_trigger(ModeTrigger *t) { pending_.push_back(t); }
  void add_pending_home_trigger(ModeTrigger *t) { pending_home_.push_back(t); }
  void add_pending_away_trigger(ModeTrigger *t) { pending_away_.push_back(t); }
  void add_pending_night_trigger(ModeTrigger *t) { pending_night_.push_back(t); }
  void add_pending_vacation_trigger(ModeTrigger *t) { pending_vacation_.push_back(t); }
  void add_pending_custom_bypass_trigger(ModeTrigger *t) { pending_custom_bypass_.push_back(t); }

  void add_armed_home_trigger(ModeTrigger *t) { armed_home_.push_back(t); }
  void add_armed_away_trigger(ModeTrigger *t) { armed_away_.push_back(t); }
  void add_armed_night_trigger(ModeTrigger *t) { armed_night_.push_back(t); }
  void add_armed_vacation_trigger(ModeTrigger *t) { armed_vacation_.push_back(t); }

  void add_armed_home_bypass_trigger(ModeTrigger *t) { armed_home_bypass_.push_back(t); }
  void add_armed_away_bypass_trigger(ModeTrigger *t) { armed_away_bypass_.push_back(t); }
  void add_armed_night_bypass_trigger(ModeTrigger *t) { armed_night_bypass_.push_back(t); }
  void add_armed_vacation_bypass_trigger(ModeTrigger *t) { armed_vacation_bypass_.push_back(t); }
  void add_armed_custom_bypass_trigger(ModeTrigger *t) { armed_custom_bypass_.push_back(t); }

  void set_initial_mode(const std::string &m) { current_mode_ = m; }

 protected:
  std::string current_mode_{"disarmed"};

  std::vector<PreModeTrigger*> intermode_trigs_;

  // Trigger vectors
  std::vector<ModeTrigger*> disarmed_;
  std::vector<ModeTrigger*> triggered_;
  std::vector<ModeTrigger*> connection_timeout_;
  std::vector<ModeTrigger*> incorrect_pin_;
  std::vector<ModeTrigger*> failed_open_sensors_;

  std::vector<ModeTrigger*> arming_;
  std::vector<ModeTrigger*> arming_home_;
  std::vector<ModeTrigger*> arming_away_;
  std::vector<ModeTrigger*> arming_night_;
  std::vector<ModeTrigger*> arming_vacation_;
  std::vector<ModeTrigger*> arming_custom_bypass_;

  std::vector<ModeTrigger*> pending_;
  std::vector<ModeTrigger*> pending_home_;
  std::vector<ModeTrigger*> pending_away_;
  std::vector<ModeTrigger*> pending_night_;
  std::vector<ModeTrigger*> pending_vacation_;
  std::vector<ModeTrigger*> pending_custom_bypass_;

  std::vector<ModeTrigger*> armed_home_;
  std::vector<ModeTrigger*> armed_away_;
  std::vector<ModeTrigger*> armed_night_;
  std::vector<ModeTrigger*> armed_vacation_;

  std::vector<ModeTrigger*> armed_home_bypass_;
  std::vector<ModeTrigger*> armed_away_bypass_;
  std::vector<ModeTrigger*> armed_night_bypass_;
  std::vector<ModeTrigger*> armed_vacation_bypass_;
  std::vector<ModeTrigger*> armed_custom_bypass_;

  void fire_for_mode_(const std::string &mode);
  void fire_intermode_();
};

template<typename... Ts>
class SetModeAction : public Action<Ts...> {
 public:
  explicit SetModeAction(TestStatesComponent *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(std::string, mode)
  void play(Ts... x) override {
    parent_->set_mode_by_name(this->mode_.value(x...));
  }
 private:
  TestStatesComponent *parent_;
};

}  // namespace test_states
}  // namespace esphome
