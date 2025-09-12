#pragma once

#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/core/automation.h"
#include <string>
#include <cstdint>

namespace esphome {
namespace test_states {

class ModeTrigger : public Trigger<> {
 public:
  // Empty; standard Trigger<> is enough.
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

  // Public API to change modes
  void set_mode(Mode m);
  void set_mode_by_name(const std::string &name);

  Mode mode() const { return mode_; }
  std::string mode_string() const;

  // Setters for triggers (called from codegen)
  void set_mode1_trigger(ModeTrigger *t) { mode1_trigger_ = t; }
  void set_mode2_trigger(ModeTrigger *t) { mode2_trigger_ = t; }
  void set_mode3_trigger(ModeTrigger *t) { mode3_trigger_ = t; }

 protected:
  Mode mode_{Mode::MODE1};

  // Optional triggers (may be nullptr)
  ModeTrigger *mode1_trigger_{nullptr};
  ModeTrigger *mode2_trigger_{nullptr};
  ModeTrigger *mode3_trigger_{nullptr};

  void fire_current_mode_trigger_();
};

}  // namespace test_states
}  // namespace esphome
