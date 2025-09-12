#include "k1_alarm_listener.h"
#include "esphome/core/log.h"

namespace esphome {
namespace k1_alarm_listener {

static const char *const TAG = "k1_alarm_listener";

void K1AlarmListener::setup() {
  if (alarm_entity_.empty()) {
    ESP_LOGE(TAG, "No alarm_entity configured.");
    return;
  }

  // Subscribe to HA state updates
  this->subscribe_homeassistant_state(
      [this](std::string state) {
        ESP_LOGD(TAG, "HA entity '%s' state update: %s",
                 this->alarm_entity_.c_str(), state.c_str());
        if (this->state_sensor_ != nullptr) {
          this->state_sensor_->publish_state(state);
        }
      },
      this->alarm_entity_);

  subscription_started_ = true;
  ESP_LOGI(TAG, "Subscribed to Home Assistant entity '%s'", alarm_entity_.c_str());
}

void K1AlarmListener::dump_config() {
  ESP_LOGCONFIG(TAG, "K1 Alarm Listener:");
  ESP_LOGCONFIG(TAG, "  Alarm Entity: %s", alarm_entity_.c_str());
  ESP_LOGCONFIG(TAG, "  Subscription: %s", subscription_started_ ? "started" : "not started");
  if (state_sensor_ == nullptr) {
    ESP_LOGCONFIG(TAG, "  Text Sensor: (not attached yet)");
  }
}

}  // namespace k1_alarm_listener
}  // namespace esphome
