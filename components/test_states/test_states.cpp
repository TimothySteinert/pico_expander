#include "test_states.h"
#include "esphome/core/log.h"

namespace esphome {
namespace test_states {

static const char *const TAG = "test_states";

void TestStatesComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Test States Component:");
  ESP_LOGCONFIG(TAG, "  Current Mode: %s", this->mode_string().c_str());
}

void TestStatesComponent::set_mode(Mode m) {
  if (m == this->mode_) return;
  this->mode_ = m;
  ESP_LOGD(TAG, "Mode changed to: %s", this->mode_string().c_str());
}

void TestStatesComponent::set_mode_by_name(const std::string &name_in) {
  std::string s = name_in;
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return (char) std::tolower(c); });
  if (s == "mode1") set_mode(Mode::MODE1);
  else if (s == "mode2") set_mode(Mode::MODE2);
  else if (s == "mode3") set_mode(Mode::MODE3);
  else {
    ESP_LOGW(TAG, "Unknown mode name '%s'", s.c_str());
  }
}

std::string TestStatesComponent::mode_string() const {
  switch (this->mode_) {
    case Mode::MODE1: return "mode1";
    case Mode::MODE2: return "mode2";
    case Mode::MODE3: return "mode3";
    default: return "unknown";
  }
}

}  // namespace test_states
}  // namespace esphome
