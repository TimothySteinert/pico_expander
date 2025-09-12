#include "k1_ready.h"
#include "esphome/core/log.h"

namespace esphome {
namespace k1_ready {

static const char *const TAG = "k1_ready";

// ---------- K1Ready (engine) ----------
void K1Ready::setup() {
  if (restore_value_) {
    pref_ = global_preferences->make_preference<std::string>(this->get_object_id_hash());
    preference_inited_ = true;
  }
  load_or_init_selection_();
  push_select_state_();
  push_ready_state_();
}

void K1Ready::dump_config() {
  ESP_LOGCONFIG(TAG, "K1 Ready Engine:");
  ESP_LOGCONFIG(TAG, "  Options (%u):", (unsigned) options_.size());
  for (auto &o : options_) ESP_LOGCONFIG(TAG, "    - %s", o.c_str());
  ESP_LOGCONFIG(TAG, "  Current Mode: %s", current_mode_.c_str());
  ESP_LOGCONFIG(TAG, "  Optimistic: %s", optimistic_ ? "YES":"NO");
  ESP_LOGCONFIG(TAG, "  Restore Value: %s", restore_value_ ? "YES":"NO");
  ESP_LOGCONFIG(TAG, "  Flags: away=%d home=%d night=%d custom=%d vacation=%d",
                (int) ready_away_, (int) ready_home_, (int) ready_night_,
                (int) ready_custom_, (int) ready_vacation_);
}

void K1Ready::register_ready_update_service() {
  this->register_service(&K1Ready::api_ready_update_, "ready_update",
                         {"armed_away",
                          "armed_home",
                          "armed_night",
                          "armed_custom_bypass",
                          "armed_vacation"});
}

void K1Ready::load_or_init_selection_() {
  if (restore_value_ && preference_inited_) {
    std::string stored;
    if (pref_.load(&stored)) {
      for (auto &o : options_) {
        if (o == stored) {
          current_mode_ = stored;
          return;
        }
      }
    }
  }
  if (!initial_option_.empty()) {
    current_mode_ = initial_option_;
  } else if (!options_.empty()) {
    current_mode_ = options_.front();
  }
}

void K1Ready::save_selection_() {
  if (!restore_value_ || !preference_inited_ || current_mode_.empty()) return;
  pref_.save(&current_mode_);
}

void K1Ready::user_select(const std::string &mode) {
  // Validate
  bool valid = false;
  for (auto &o : options_) {
    if (o == mode) { valid = true; break; }
  }
  if (!valid) {
    ESP_LOGW(TAG, "Rejecting invalid mode '%s'", mode.c_str());
    return;
  }
  if (mode == current_mode_) return;
  current_mode_ = mode;
  save_selection_();
  ESP_LOGD(TAG, "Mode changed to '%s'", current_mode_.c_str());
  push_select_state_();
  push_ready_state_();
}

bool K1Ready::ready_for_mode(const std::string &mode) const {
  if (mode == "armed_away") return ready_away_;
  if (mode == "armed_home") return ready_home_;
  if (mode == "armed_night") return ready_night_;
  if (mode == "armed_vacation") return ready_vacation_;
  if (mode == "armed_custom_bypass") return ready_custom_;
  return false;
}

void K1Ready::push_select_state_() {
  if (select_ != nullptr) select_->publish_state(current_mode_);
}

void K1Ready::push_ready_state_() {
  if (ready_sensor_ != nullptr) ready_sensor_->update_from_engine(current_mode_ready());
}

void K1Ready::api_ready_update_(bool armed_away,
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
    ESP_LOGD(TAG, "Readiness updated: away=%d home=%d night=%d custom=%d vacation=%d",
             (int)ready_away_, (int)ready_home_, (int)ready_night_,
             (int)ready_custom_, (int)ready_vacation_);
    push_ready_state_();
  }
}

// ---------- Facade Select ----------
select::SelectTraits K1ReadySelect::get_traits() {
  select::SelectTraits traits;
  if (engine_ != nullptr)
    traits.set_options(engine_->options());
  return traits;
}

void K1ReadySelect::control(const std::string &value) {
  if (engine_ != nullptr)
    engine_->user_select(value);
}

// ---------- Binary Sensor ----------
/* (K1ReadyBinarySensor has no custom logic; engine invokes update_from_engine) */

}  // namespace k1_ready
}  // namespace esphome
