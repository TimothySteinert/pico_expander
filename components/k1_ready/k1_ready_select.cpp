#include "k1_ready_select.h"
#include "esphome/core/log.h"

namespace esphome {
namespace k1_ready {

static const char *const TAG = "k1_ready";

void K1ReadySelect::setup() {
  if (restore_value_) {
    pref_ = global_preferences->make_preference<std::string>(this->get_object_id_hash());
    std::string stored;
    if (pref_.load(&stored)) {
      for (auto &o : options_) {
        if (o == stored) {
          restored_value_ = stored;
          has_restored_ = true;
          break;
        }
      }
    }
  }
  publish_initial_state_();
  // Register service (if not already via codegen - but we separate registration to explicit call)
  // (No-op here; codegen will call register_ready_update_service())
  publish_ready_state_();
}

void K1ReadySelect::register_ready_update_service() {
  this->register_service(&K1ReadySelect::api_ready_update_, "ready_update",
                         {"armed_away", "armed_home", "armed_night", "armed_custom_bypass", "armed_vacation"});
}

void K1ReadySelect::dump_config() {
  ESP_LOGCONFIG(TAG, "K1 Ready Select:");
  ESP_LOGCONFIG(TAG, "  Optimistic: %s", optimistic_ ? "YES" : "NO");
  ESP_LOGCONFIG(TAG, "  Restore Value: %s", restore_value_ ? "YES" : "NO");
  if (!initial_option_.empty())
    ESP_LOGCONFIG(TAG, "  Initial Option: %s", initial_option_.c_str());
  ESP_LOGCONFIG(TAG, "  Options (%u):", (unsigned) options_.size());
  for (auto &o : options_)
    ESP_LOGCONFIG(TAG, "    - %s", o.c_str());
  ESP_LOGCONFIG(TAG, "  Current Selection: %s", this->state.c_str());
  ESP_LOGCONFIG(TAG, "  Readiness Flags:");
  ESP_LOGCONFIG(TAG, "    armed_away: %s", ready_away_ ? "READY" : "NOT READY");
  ESP_LOGCONFIG(TAG, "    armed_home: %s", ready_home_ ? "READY" : "NOT READY");
  ESP_LOGCONFIG(TAG, "    armed_night: %s", ready_night_ ? "READY" : "NOT READY");
  ESP_LOGCONFIG(TAG, "    armed_custom_bypass: %s", ready_custom_ ? "READY" : "NOT READY");
  ESP_LOGCONFIG(TAG, "    armed_vacation: %s", ready_vacation_ ? "READY" : "NOT READY");
}

select::SelectTraits K1ReadySelect::get_traits() {
  select::SelectTraits traits;
  traits.set_options(options_);
  return traits;
}

void K1ReadySelect::publish_initial_state_() {
  std::string chosen;
  if (has_restored_) {
    chosen = restored_value_;
  } else if (!initial_option_.empty()) {
    chosen = initial_option_;
  } else if (!options_.empty()) {
    chosen = options_.front();
  }
  if (!chosen.empty()) {
    this->publish_state(chosen);
  }
}

void K1ReadySelect::control(const std::string &value) {
  // Validate option
  bool valid = false;
  for (auto &o : options_) {
    if (o == value) { valid = true; break; }
  }
  if (!valid) {
    ESP_LOGW(TAG, "Ignoring invalid selection '%s'", value.c_str());
    return;
  }

  // Optimistic immediate publish (there is no external confirmation channel here)
  this->publish_state(value);

  if (restore_value_)
    save_();

  publish_ready_state_();
}

void K1ReadySelect::save_() {
  if (!restore_value_ || !pref_.is_initialized()) return;
  const std::string &cur = this->state;
  if (cur.empty()) return;
  pref_.save(&cur);
}

void K1ReadySelect::api_ready_update_(bool armed_away,
                                      bool armed_home,
                                      bool armed_night,
                                      bool armed_custom_bypass,
                                      bool armed_vacation) {
  bool changed =
      (ready_away_ != armed_away) ||
      (ready_home_ != armed_home) ||
      (ready_night_ != armed_night) ||
      (ready_custom_ != armed_custom_bypass) ||
      (ready_vacation_ != armed_vacation);

  ready_away_ = armed_away;
  ready_home_ = armed_home;
  ready_night_ = armed_night;
  ready_custom_ = armed_custom_bypass;
  ready_vacation_ = armed_vacation;

  if (changed) {
    ESP_LOGD(TAG, "Readiness flags updated: away=%d home=%d night=%d custom=%d vacation=%d",
             (int)ready_away_, (int)ready_home_, (int)ready_night_, (int)ready_custom_, (int)ready_vacation_);
    publish_ready_state_();
  }
}

bool K1ReadySelect::ready_for_mode_(const std::string &mode) const {
  if (mode == "armed_away") return ready_away_;
  if (mode == "armed_home") return ready_home_;
  if (mode == "armed_night") return ready_night_;
  if (mode == "armed_vacation") return ready_vacation_;
  if (mode == "armed_custom_bypass") return ready_custom_;
  return false;
}

bool K1ReadySelect::current_mode_ready() const {
  return ready_for_mode_(this->state);
}

void K1ReadySelect::publish_ready_state_() {
  if (ready_binary_sensor_ == nullptr) return;
  bool ready = current_mode_ready();
  ready_binary_sensor_->update_from_parent(ready);
}

}  // namespace k1_ready
}  // namespace esphome
