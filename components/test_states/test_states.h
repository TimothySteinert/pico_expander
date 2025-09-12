#pragma once
#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/core/log.h"
#include <map>
#include <vector>
#include <string>
#include <algorithm>

namespace esphome {
namespace test_states {

class TestStatesComponent;

// Trigger for entering a mode
class ModeTrigger : public Trigger<> {
 public:
  ModeTrigger() = default;
};

class TestStatesComponent : public Component {
 public:
  void setup() override {}
  void loop() override {}
  void dump_config() override;

  void set_mode_by_name(const std::string &name);
  const std::string &current_mode_string() const { return current_mode_; }

  void add_mode_trigger(const std::string &mode, ModeTrigger *t);
  void set_initial_mode(const std::string &m) { current_mode_ = m; }

 protected:
  std::string current_mode_{};
  std::map<std::string, std::vector<ModeTrigger *>> mode_triggers_;

  void fire_mode_(const std::string &mode);
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
