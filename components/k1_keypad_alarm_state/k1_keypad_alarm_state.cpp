#include "k1_keypad_alarm_state.h"
#include "esphome/core/log.h"

namespace esphome {
namespace k1_keypad_alarm_state {

static const char *const TAG = "k1_keypad_alarm_state";

void K1KeypadAlarmState::setup() {
  ESP_LOGI(TAG, "Subscribing to HA entity: %s", this->entity_id_.c_str());

  // Requires api: homeassistant_states: true in YAML
  this->subscribe_homeassistant_state(&K1KeypadAlarmState::on_ha_state_, this->entity_id_);
}

void K1KeypadAlarmState::dump_config() {
  ESP_LOGCONFIG(TAG, "K1 Keypad Alarm State:");
  ESP_LOGCONFIG(TAG, "  Entity ID: %s", this->entity_id_.c_str());
}

void K1KeypadAlarmState::on_ha_state_(std::string state) {
  std::string mapped = (state == "unavailable" || state == "unknown")
                         ? "connection_timed_out"
                         : state;
  ESP_LOGD(TAG, "Mapped state: %s -> %s", state.c_str(), mapped.c_str());
  this->publish_state(mapped);
}

}  // namespace k1_keypad_alarm_state
}  // namespace esphome
