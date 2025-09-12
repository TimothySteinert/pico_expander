#include "test_states.h"
#include "esphome/core/log.h"

namespace esphome {
namespace test_states {

static const char *const TAG = "test_states";

void TestStatesComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Test States Component:");
  ESP_LOGCONFIG(TAG, "  Current Mode: %s", current_mode_.c_str());
  ESP_LOGCONFIG(TAG, "  intermode triggers: %u", (unsigned) intermode_trigs_.size());
  ESP_LOGCONFIG(TAG, "  mode1 triggers: %u", (unsigned) mode1_trigs_.size());
  ESP_LOGCONFIG(TAG, "  mode2 triggers: %u", (unsigned) mode2_trigs_.size());
  ESP_LOGCONFIG(TAG, "  mode3 triggers: %u", (unsigned) mode3_trigs_.size());
  ESP_LOGCONFIG(TAG, "  mode4 triggers: %u", (unsigned) mode4_trigs_.size());
  ESP_LOGCONFIG(TAG, "  mode5 triggers: %u", (unsigned) mode5_trigs_.size());
}

void TestStatesComponent::fire_intermode_() {
  for (auto *t : intermode_trigs_) if (t) t->trigger();
}

void TestStatesComponent::set_mode_by_name(const std::string &name_in) {
  if (name_in.empty()) return;
  std::string s = name_in;
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c){ return (char) std::tolower(c); });
  if (s == current_mode_) return;

  // Run global intermode actions first
  fire_intermode_();

  std::string old = current_mode_;
  current_mode_ = s;
  ESP_LOGI(TAG, "Mode changed: '%s' -> '%s'", old.c_str(), current_mode_.c_str());
  fire_for_mode_(current_mode_);
}

void TestStatesComponent::fire_for_mode_(const std::string &mode) {
  const std::vector<ModeTrigger*> *vec = nullptr;
  if (mode == "mode1") vec = &mode1_trigs_;
  else if (mode == "mode2") vec = &mode2_trigs_;
  else if (mode == "mode3") vec = &mode3_trigs_;
  else if (mode == "mode4") vec = &mode4_trigs_;
  else if (mode == "mode5") vec = &mode5_trigs_;
  if (!vec) return;
  for (auto *t : *vec) if (t) t->trigger();
}

}  // namespace test_states
}  // namespace esphome
