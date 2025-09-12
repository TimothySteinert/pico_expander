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

  // Register triggers
  void add_inter_mode_trigger(PreModeTrigger *t) { intermode_trigs_.push_back(t); }
  void add_mode1_trigger(ModeTrigger *t) { mode1_trigs_.push_back(t); }
  void add_mode2_trigger(ModeTrigger *t) { mode2_trigs_.push_back(t); }
  void add_mode3_trigger(ModeTrigger *t) { mode3_trigs_.push_back(t); }
  void add_mode4_trigger(ModeTrigger *t) { mode4_trigs_.push_back(t); }
  void add_mode5_trigger(ModeTrigger *t) { mode5_trigs_.push_back(t); }

  void set_initial_mode(const std::string &m) { current_mode_ = m; }

 protected:
  std::string current_mode_{"mode1"};

  std::vector<PreModeTrigger*> intermode_trigs_;
  std::vector<ModeTrigger*> mode1_trigs_;
  std::vector<ModeTrigger*> mode2_trigs_;
  std::vector<ModeTrigger*> mode3_trigs_;
  std::vector<ModeTrigger*> mode4_trigs_;
  std::vector<ModeTrigger*> mode5_trigs_;

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
