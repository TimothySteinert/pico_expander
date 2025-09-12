#pragma once

#include "esphome/core/component.h"
#include <string>
#include <algorithm>

namespace esphome {
namespace test_states {

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

  void set_mode(Mode m);
  void set_mode_by_name(const std::string &name);
  Mode mode() const { return mode_; }
  std::string mode_string() const;

 protected:
  Mode mode_{Mode::MODE1};
};

}  // namespace test_states
}  // namespace esphome
