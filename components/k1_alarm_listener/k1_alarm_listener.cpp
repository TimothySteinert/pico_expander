#include "k1_alarm_listener.h"
#include "esphome/core/log.h"
#include <algorithm>

namespace esphome {
namespace k1_alarm_listener {

static const char *const TAG = "k1_alarm_listener";

// Recognized states (unavailable/unknown remapped)
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

  this->subscribe_homeassistant_state(&K1AlarmListener::ha_state_callback_, this->alarm_entity_);
  subscription_started_ = true;
  ESP_LOGI(TAG, "Subscribed to Home Assistant entity '%s'", alarm_entity_.c_str());

  // Initialize connection sensor reading immediately
  bool connected = api_connected_now_();
  last_api_connected_ = connected;
  update_connection_sensor_(connected);
  if (!connected) {
    enforce_connection_state_();
  }
}

void K1AlarmListener::loop() {
  bool connected = api_connected_now_();
  if (connected != last_api_connected_) {
    ESP_LOGI(TAG, "API connection state changed: %s", connected ? "CONNECTED" : "DISCONNECTED");
    last_api_connected_ = connected;
    update_connection_sensor_(connected);
    enforce_connection_state_();
  }
}

bool K1AlarmListener::api_connected_now_() const {
#ifdef USE_API
  if (api::global_api_server == nullptr) return false;
  // is_connected() returns true if at least one client (e.g. Home Assistant) is connected.
  return api::global_api_server->is_connected();
#else
  return true;
#endif
}

void K1AlarmListener::update_connection_sensor_(bool connected) {
  if (ha_connection_sensor_ == nullptr) return;
  // Binary sensor ON means "connected" (like status platform).
  ha_connection_sensor_->publish_state(connected);
}

void K1AlarmListener::enforce_connection_state_() {
  if (!last_api_connected_) {
    if (state_sensor_ != nullptr && last_published_state_ != "connection_timeout") {
      ESP_LOGW(TAG, "Forcing state to connection_timeout (API disconnected)");
      state_sensor_->publish_state("connection_timeout");
      last_published_state_ = "connection_timeout";
    }
  } else {
    // On reconnection we wait for next HA entity state update
    ESP_LOGI(TAG, "API reconnected; awaiting alarm entity state refresh.");
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
    return "connection_timeout";
  return raw;
}

void K1AlarmListener::ha_state_callback_(std::string state) {
  if (!last_api_connected_) {
    ESP_LOGV(TAG, "Ignoring alarm state '%s' while API disconnected", state.c_str());
    return;
  }

  std::string mapped = map_alarm_state_(state);
  if (state_sensor_ != nullptr) {
    if (mapped != last_published_state_) {
      if (!is_known_alarm_state_(mapped) && mapped != "connection_timeout") {
        ESP_LOGW(TAG, "Non-standard alarm state '%s' (publishing as-is)", mapped.c_str());
      } else {
        ESP_LOGD(TAG, "Alarm state update: raw='%s' publish='%s'", state.c_str(), mapped.c_str());
      }
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
  ESP_LOGCONFIG(TAG, "  API Connected (last seen): %s", last_api_connected_ ? "YES" : "NO");
  if (state_sensor_ == nullptr) {
    ESP_LOGCONFIG(TAG, "  Text Sensor: (not attached yet)");
  } else {
    ESP_LOGCONFIG(TAG, "  Last Published: %s",
                  last_published_state_.empty() ? "(none)" : last_published_state_.c_str());
  }
  ESP_LOGCONFIG(TAG, "  HA Connection Binary Sensor: %s",
                ha_connection_sensor_ ? "present" : "absent");
}

}  // namespace k1_alarm_listener
}  // namespace esphome
