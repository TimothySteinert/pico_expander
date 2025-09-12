#include "test_states.h"
#include "esphome/core/log.h"

namespace esphome {
namespace test_states {

static const char *const TAG = "test_states";

void TestStatesComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Test States Component:");
  ESP_LOGCONFIG(TAG, "  Current Mode: %s", current_mode_.c_str());
  ESP_LOGCONFIG(TAG, "  Intermode triggers: %u", (unsigned) intermode_trigs_.size());
  // (Optional) You could log counts per mode here if desired.
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

  fire_intermode_();  // run pre-transition actions first

  std::string old = current_mode_;
  current_mode_ = s;
  ESP_LOGI(TAG, "Mode changed: '%s' -> '%s'", old.c_str(), current_mode_.c_str());
  fire_for_mode_(current_mode_);
}

void TestStatesComponent::fire_for_mode_(const std::string &mode) {
  const std::vector<ModeTrigger*> *vec = nullptr;

  if (0) {}
  else if (mode == "disarmed") vec = &disarmed_;
  else if (mode == "triggered") vec = &triggered_;
  else if (mode == "connection_timeout") vec = &connection_timeout_;
  else if (mode == "incorrect_pin") vec = &incorrect_pin_;
  else if (mode == "failed_open_sensors") vec = &failed_open_sensors_;

  else if (mode == "arming") vec = &arming_;
  else if (mode == "arming_home") vec = &arming_home_;
  else if (mode == "arming_away") vec = &arming_away_;
  else if (mode == "arming_night") vec = &arming_night_;
  else if (mode == "arming_vacation") vec = &arming_vacation_;
  else if (mode == "arming_custom_bypass") vec = &arming_custom_bypass_;

  else if (mode == "pending") vec = &pending_;
  else if (mode == "pending_home") vec = &pending_home_;
  else if (mode == "pending_away") vec = &pending_away_;
  else if (mode == "pending_night") vec = &pending_night_;
  else if (mode == "pending_vacation") vec = &pending_vacation_;
  else if (mode == "pending_custom_bypass") vec = &pending_custom_bypass_;

  else if (mode == "armed_home") vec = &armed_home_;
  else if (mode == "armed_away") vec = &armed_away_;
  else if (mode == "armed_night") vec = &armed_night_;
  else if (mode == "armed_vacation") vec = &armed_vacation_;

  else if (mode == "armed_home_bypass") vec = &armed_home_bypass_;
  else if (mode == "armed_away_bypass") vec = &armed_away_bypass_;
  else if (mode == "armed_night_bypass") vec = &armed_night_bypass_;
  else if (mode == "armed_vacation_bypass") vec = &armed_vacation_bypass_;
  else if (mode == "armed_custom_bypass") vec = &armed_custom_bypass_;

  if (!vec) {
    ESP_LOGW(TAG, "No triggers vector for mode '%s'", mode.c_str());
    return;
  }
  for (auto *t : *vec)
    if (t) t->trigger();
}

}  // namespace test_states
}  // namespace esphome
