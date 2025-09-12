#pragma once
#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/core/log.h"
#include <vector>
#include <string>
#include <cstdint>

namespace esphome {
namespace test_states {

class TestStatesComponent;  // forward

// A simple trigger tied to a parent component (not strictly needed now, but extensible later)
class ModeTrigger : public Trigger<> {
 public:
  explicit ModeTrigger(TestStatesComponent *parent) : parent_(parent) {}
 protected:
  TestStatesComponent *parent_;
};

class TestStatesComponent : public Component {
 public:
  enum class Mode : uint8_t {
    MODE1 = 0,
    MODE2,
    MODE3
  };

  void setup() override {}
  void loop() override {}
  void dump_config() override;

  // Change mode
  void set_mode(Mode m);
  void set_mode_by_name(const std::string &name);

  Mode mode() const { return mode_; }
  std::string mode_string() const;

  // Called from codegen to register triggers
  void add_mode1_trigger(ModeTrigger *t) { mode1_triggers_.push_back(t); }
  void add_mode2_trigger(ModeTrigger *t) { mode2_triggers_.push_back(t); }
  void add_mode3_trigger(ModeTrigger *t) { mode3_triggers_.push_back(t); }

 protected:
  Mode mode_{Mode::MODE1};

  std::vector<ModeTrigger*> mode1_triggers_;
  std::vector<ModeTrigger*> mode2_triggers_;
  std::vector<ModeTrigger*> mode3_triggers_;

  void fire_triggers_for_mode_(Mode m);
};

}  // namespace test_states
}  // namespace esphome
