#include "k1_keypad_alarm_state.h"
#include "esphome/core/log.h"
#include "esphome/components/homeassistant/text_sensor/homeassistant_text_sensor.h"

namespace esphome {
namespace k1_keypad_alarm_state {

static const char *const TAG = "k1_keypad_alarm_state";

void K1KeypadAlarmState::setup() {
  ESP_LOGI(TAG, "Subscribing to HA entity: %s", this->entity_id_.c_str());

  auto *ha_sensor = new homeassistant::HomeassistantTextSensor();
  ha_sensor->set_entity_id(this->entity_id_);
  ha_sensor->add_on_state_callback([this](std::string state) {
    this->process_state(state);
  });

  App.register_component(ha_sensor);
}

void K1KeypadAlarmState::dump_config() {
  ESP_LOGCONFIG(TAG, "K1 Keypad Alarm State:");
  ESP_LOGCONFIG(TAG, "  Entity ID: %s", this->entity_id_.c_str());
}

void K1KeypadAlarmState::loop() {
  // nothing – all updates come via callback
}

void K1KeypadAlarmState::process_state(const std::string &state) {
  std::string mapped;
  if (state == "unavailable" || state == "unknown") {
    mapped = "connection_timed_out";
  } else {
    mapped = state;
  }

  ESP_LOGD(TAG, "Mapped state: %s -> %s", state.c_str(), mapped.c_str());
  this->publish_state(mapped);
}

}  // namespace k1_keypad_alarm_state
}  // namespace esphome
