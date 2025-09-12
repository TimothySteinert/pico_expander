#include "k1_alarm_listener.h"
#include "esphome/core/log.h"
#include <algorithm>

namespace esphhome {  // (typo intentionally fixed below, keep namespace correct)
}
namespace esphome {
namespace k1_alarm_listener {

static const char *const TAG = "k1_alarm_listener";

// Canonical states we recognize (unavailable/unknown get remapped)
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
    "unavailable",
    "unknown"};

void K1AlarmListener::setup() {
  if (alarm_entity_.empty()) {
    ESP_LOGE(TAG, "No alarm_entity configured.");
    return;
  }

  // Subscribe to HA alarm entity state updates
  this->subscribe_homeassistant_state(&K1AlarmListener::ha_state_callback_, this->alarm_entity_);
  subscription_started_ = true;
  ESP_LOGI(TAG, "Subscribed to Home Assistant entity '%s'", alarm_entity_.c_str());

  // Listen to HA availability status (status binary sensor) if provided
  if (ha_status_sensor_ != nullptr) {
    ha_status_sensor_->add_on_state_callback([this](float f) { this->ha_status_changed_(f); });
    // Evaluate immediately with current state (NOTE: binary sensor publishes after setup;
    // if not yet available treat as disconnected until callback triggers)
    this->evaluate_connection_state_();
  }
}

bool K1AlarmListener::is_known_alarm_state_(const std::string &s) const {
  for (auto *st : KNOWN_STATES) {
    if (s == st) return true;
  }
  return false;
}

std::string K1AlarmListener::map_alarm_state_(const std::string &raw_in) const {
  std::string raw = raw_in;
  std::transform(raw.begin(), raw.end(), raw.begin(), [](unsigned char c) { return (char) std::tolower(c); });

  if (raw == "unavailable" || raw == "unknown")
    return "connection_timeout";  // remap

  // Pass-through known states; unknown custom states pass through unchanged
  return raw;
}

bool K1AlarmListener::is_ha_connected_() const {
  if (ha_status_sensor_ == nullptr) {
    // If user did not provide a status binary sensor, assume connected (legacy behavior)
    return true;
  }
  return ha_status_sensor_->state;  // true = connected
}

void K1AlarmListener::evaluate_connection_state_() {
  bool connected = this->is_ha_connected_();
  if (!connected) {
    if (state_sensor_ != nullptr && last_published_state_ != "connection_timeout") {
      state_sensor_->publish_state("connection_timeout");
      last_published_state_ = "connection_timeout";
      ESP_LOGW(TAG, "HA disconnected -> publishing connection_timeout");
    }
  } else {
    // On reconnect we wait for next HA entity state update; do not publish stale state here.
    ESP_LOGI(TAG, "HA connection restored; waiting for alarm entity update.");
  }
}

void K1AlarmListener::ha_status_changed_(float) {
  // Called whenever the status binary sensor toggles
  this->evaluate_connection_state_();
}

void K1AlarmListener::ha_state_callback_(std::string state) {
  // Ignore alarm state updates if HA currently marked disconnected
  if (!this->is_ha_connected_()) {
    ESP_LOGV(TAG, "Received alarm state '%s' while HA disconnected -> ignored", state.c_str());
    return;
  }

  std::string mapped = map_alarm_state_(state);

  if (state_sensor_ != nullptr) {
    if (mapped != last_published_state_) {
      ESP_LOGD(TAG, "Alarm state update: raw='%s' publish='%s'", state.c_str(), mapped.c_str());
      state_sensor_->publish_state(mapped);
      last_published_state_ = mapped;
    } else {
      ESP_LOGV(TAG, "Alarm state unchanged (%s)", mapped.c_str());
    }
  }
}

void K1AlarmListener::dump_config() {
  ESP_LOGCONFIG(TAG, "K1 Alarm Listener:");
  ESP_LOGCONFIG(TAG, "  Alarm Entity: %s", alarm_entity_.c_str());
  ESP_LOGCONFIG(TAG, "  Subscription: %s", subscription_started_ ? "started" : "not started");
  ESP_LOGCONFIG(TAG, "  Mapping: unavailable|unknown -> connection_timeout");
  if (ha_status_sensor_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  HA Status Sensor Linked: YES (current=%s)",
                  is_ha_connected_() ? "connected" : "disconnected");
  } else {
    ESP_LOGCONFIG(TAG, "  HA Status Sensor Linked: NO (assuming connected)");
  }
  if (state_sensor_ == nullptr) {
    ESP_LOGCONFIG(TAG, "  Text Sensor: (not attached yet)");
  } else {
    ESP_LOGCONFIG(TAG, "  Last Published: %s",
                  last_published_state_.empty() ? "(none)" : last_published_state_.c_str());
  }
}

}  // namespace k1_alarm_listener
}  // namespace esphome
