#include "test_states.h"
#include "esphome/core/log.h"

namespace esphome {
namespace test_states {

static const char *const TAG = "test_states";

void TestStatesComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Test States Component:");
  ESP_LOGCONFIG(TAG, "  Current Mode: %s", current_mode_.c_str());
  if (!mode_triggers_.empty()) {
    ESP_LOGCONFIG(TAG, "  Registered modes:");
    for (auto &kv : mode_triggers_) {
      ESP_LOGCONFIG(TAG, "    %s -> %u trigger(s)", kv.first.c_str(), (unsigned) kv.second.size());
    }
  }
}

void TestStatesComponent::add_mode_trigger(const std::string &mode, ModeTrigger *t) {
  std::string m = mode;
  std::transform(m.begin(), m.end(), m.begin(), [](unsigned char c){ return (char) std::tolower(c); });
  mode_triggers_[m].push_back(t);
}

void TestStatesComponent::set_mode_by_name(const std::string &name_in) {
  std::string s = name_in;
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return (char) std::tolower(c); });

  if (s == current_mode_) return;
  std::string old = current_mode_;
  current_mode_ = s;
  ESP_LOGI(TAG, "Mode changed: '%s' -> '%s'", old.c_str(), current_mode_.c_str());
  ESP_LOGI(TAG, "I'm in mode %s", current_mode_.c_str());
  fire_mode_(current_mode_);
}

void TestStatesComponent::fire_mode_(const std::string &mode) {
  auto it = mode_triggers_.find(mode);
  if (it == mode_triggers_.end()) return;
  for (auto *t : it->second) {
    if (t) t->trigger();
  }
}

}  // namespace test_states
}  // namespace esphome
