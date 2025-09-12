#include "k1_alarm_listener.h"
#include "esphome/core/log.h"
#include <algorithm>

namespace esphome {
namespace k1_alarm_listener {

static const char *const TAG = "k1_alarm_listener";

// List of documented alarm states we simply pass through (except unavailable/unknown which are remapped)
static const char *const KNOWN_STATES[] = {
    "disarmed",
    "arming",
    "armed_away",
    "armed_home",
    "armed_night",
    "armed_vacation",
    "armed_custom_bypass",
    "pending",
    "triggered",
    "unavailable",  // will be remapped
    "unknown"       // will be remapped
};

void K1AlarmListener::setup() {
  if (alarm_entity_.empty()) {
    ESP_LOGE(TAG, "No alarm_entity configured.");
    return;
  }

  this->subscribe_homeassistant_state(&K1AlarmListener::ha_state_callback_, this->alarm_entity_);
  subscription_started_ = true;
  ESP_LOGI(TAG, "Subscribed to Home Assistant entity '%s'", alarm_entity_.c_str());
}

bool K1AlarmListener::is_known_alarm_state_(const std::string &s) const {
  for (auto *st : KNOWN_STATES) {
    if (s == st) return true;
  }
  return false;
}

void K1AlarmListener::ha_state_callback_(std::string state) {
  // Normalize to lowercase just in case (HA usually already provides lower)
  std::string lowered = state;
  std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                 [](unsigned char c) { return (char) std::tolower(c); });

  std::string publish_state = lowered;

  if (lowered == "unavailable" || lowered == "unknown") {
    publish_state = "connection_timeout";
    ESP_LOGW(TAG,
             "Received '%s' from HA for '%s' -> publishing '%s' instead",
             lowered.c_str(), alarm_entity_.c_str(), publish_state.c_str());
  } else if (!is_known_alarm_state_(lowered)) {
    // Optional: still forward unknown custom states, but log them.
    ESP_LOGW(TAG,
             "Received non-standard alarm state '%s' (forwarding unchanged)",
             lowered.c_str());
  } else {
    ESP_LOGD(TAG, "Alarm state update: %s", publish_state.c_str());
  }

  if (this->state_sensor_ != nullptr) {
    this->state_sensor_->publish_state(publish_state);
  }
}

void K1AlarmListener::dump_config() {
  ESP_LOGCONFIG(TAG, "K1 Alarm Listener:");
  ESP_LOGCONFIG(TAG, "  Alarm Entity: %s", alarm_entity_.c_str());
  ESP_LOGCONFIG(TAG, "  Subscription: %s", subscription_started_ ? "started" : "not started");
  ESP_LOGCONFIG(TAG, "  State mapping: unavailable|unknown -> connection_timeout");
  if (state_sensor_ == nullptr) {
    ESP_LOGCONFIG(TAG, "  Text Sensor: (not attached yet)");
  }
}

}  // namespace k1_alarm_listener
}  // namespace esphome
