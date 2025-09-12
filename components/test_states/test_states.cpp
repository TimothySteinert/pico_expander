#include "test_states.h"
#include "esphome/core/log.h"
#include <algorithm>

namespace esphome {
namespace test_states {

static const char *const TAG = "test_states";

void TestStatesComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Test States Component:");
  ESP_LOGCONFIG(TAG, "  Current Mode: %s", this->mode_string().c_str());
  ESP_LOGCONFIG(TAG, "  Triggers attached:");
  ESP_LOGCONFIG(TAG, "    mode1: %s", mode1_trigger_ ? "YES" : "NO");
  ESP_LOGCONFIG(TAG, "    mode2: %s", mode2_trigger_ ? "YES" : "NO");
  ESP_LOGCONFIG(TAG, "    mode3: %s", mode3_trigger_ ? "YES" : "NO");
}

void TestStatesComponent::set_mode(Mode m) {
  if (m == this->mode_) return;
  this->mode_ = m;
  ESP_LOGI(TAG, "Mode changed -> %s", this->mode_string().c_str());
  // For this first test we just log per your request
  switch (mode_) {
    case Mode::MODE1:
      ESP_LOGI(TAG, "I'm in mode 1");
      break;
    case Mode::MODE2:
      ESP_LOGI(TAG, "I'm in mode 2");
      break;
    case Mode::MODE3:
      ESP_LOGI(TAG, "I'm in mode 3");
      break;
  }
  fire_current_mode_trigger_();
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

void TestStatesComponent::fire_current_mode_trigger_() {
  switch (mode_) {
    case Mode::MODE1:
      if (mode1_trigger_) mode1_trigger_->trigger();
      break;
    case Mode::MODE2:
      if (mode2_trigger_) mode2_trigger_->trigger();
      break;
    case Mode::MODE3:
      if (mode3_trigger_) mode3_trigger_->trigger();
      break;
  }
}

}  // namespace test_states
}  // namespace esphome
