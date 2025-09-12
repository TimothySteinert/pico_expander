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

class ModeTrigger : public Trigger<> {
 public:
  explicit ModeTrigger(TestStatesComponent *parent) : parent_(parent) {}
 protected:
  TestStatesComponent *parent_;
};

class TestStatesComponent : public Component {
 public:
  void setup() override {}
  void loop() override {}
  void dump_config() override;

  void set_mode_by_name(const std::string &name);
  const std::string &current_mode_string() const { return current_mode_; }

  void add_mode_trigger(const std::string &mode, ModeTrigger *t);

 protected:
  std::string current_mode_{"mode1"};
  std::map<std::string, std::vector<ModeTrigger *>> mode_triggers_;

  void fire_mode_(const std::string &mode);
};

template<typename... Ts>
class SetModeAction : public Action<Ts...> {
 public:
  explicit SetModeAction(TestStatesComponent *parent) : parent_(parent) {}
  void set_mode(const std::string &m) { mode_ = m; }
  void play(Ts... x) override { parent_->set_mode_by_name(mode_); }
 protected:
  TestStatesComponent *parent_;
  std::string mode_;
};

}  // namespace test_states
}  // namespace esphome
