#include "k1_alarm_listener.h"
#include "esphome/core/log.h"
#include <algorithm>
#include <cctype>

namespace esphome {
namespace k1_alarm_listener {

static const char *const TAG = "k1_alarm_listener";

// Base states we know (before inference)
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
    "unknown"
};

// ---------- Setup ----------
void K1AlarmListener::setup() {
  if (alarm_entity_.empty()) {
    ESP_LOGE(TAG, "No alarm_entity configured.");
    return;
  }

  // Subscribe to base state
  this->subscribe_homeassistant_state(&K1AlarmListener::ha_state_callback_, this->alarm_entity_);
  subscription_started_ = true;
  ESP_LOGI(TAG, "Subscribed to entity '%s' state", alarm_entity_.c_str());

  // Subscribe to attributes (Alarmo style). If not provided, callbacks just never fire.
  this->subscribe_homeassistant_state(&K1AlarmListener::arm_mode_attr_callback_, this->alarm_entity_, "arm_mode");
  this->subscribe_homeassistant_state(&K1AlarmListener::next_state_attr_callback_, this->alarm_entity_, "next_state");
  this->subscribe_homeassistant_state(&K1AlarmListener::bypassed_attr_callback_, this->alarm_entity_, "bypassed_sensors");
  ESP_LOGI(TAG, "Subscribed to attributes: arm_mode, next_state, bypassed_sensors");

  bool connected = api_connected_now_();
  last_api_connected_ = connected;
  update_connection_sensor_(connected);
  if (!connected) enforce_connection_state_();
}

void K1AlarmListener::loop() {
  bool connected = api_connected_now_();
  if (connected != last_api_connected_) {
    ESP_LOGI(TAG, "API connection %s", connected ? "RESTORED" : "LOST");
    last_api_connected_ = connected;
    update_connection_sensor_(connected);
    enforce_connection_state_();
  }
}

// ---------- Connection Helpers ----------
bool K1AlarmListener::api_connected_now_() const {
#ifdef USE_API
  if (api::global_api_server == nullptr) return false;
  return api::global_api_server->is_connected();
#else
  return true;
#endif
}

void K1AlarmListener::update_connection_sensor_(bool connected) {
  if (ha_connection_sensor_ != nullptr)
    ha_connection_sensor_->publish_state(connected);
}

void K1AlarmListener::enforce_connection_state_() {
  if (!last_api_connected_) {
    publish_state_if_changed_(CONNECTION_TIMEOUT_STATE);
  } else {
    ESP_LOGD(TAG, "API connected; waiting for next state/attr update.");
  }
}

// ---------- Attribute Callbacks ----------
static bool attr_has_value_(const std::string &v) {
  if (v.empty()) return false;
  std::string low;
  low.reserve(v.size());
  for (char c: v) low.push_back((char) std::tolower((unsigned char)c));
  return !(low == "null" || low == "none");
}

void K1AlarmListener::arm_mode_attr_callback_(std::string value) {
  attr_.arm_mode = trim_copy_(value);
  attr_.arm_mode_seen = attr_has_value_(attr_.arm_mode);
  if (attr_.arm_mode_seen) attributes_supported_ = true;
  maybe_publish_updated_state_();
}

void K1AlarmListener::next_state_attr_callback_(std::string value) {
  attr_.next_state = trim_copy_(value);
  attr_.next_state_seen = attr_has_value_(attr_.next_state);
  if (attr_.next_state_seen) attributes_supported_ = true;
  maybe_publish_updated_state_();
}

void K1AlarmListener::bypassed_attr_callback_(std::string value) {
  attr_.bypassed_sensors = trim_copy_(value);
  attr_.bypassed_seen = attr_has_value_(attr_.bypassed_sensors);
  if (attr_.bypassed_seen) attributes_supported_ = true;
  maybe_publish_updated_state_();
}

// ---------- Base State Callback ----------
void K1AlarmListener::ha_state_callback_(std::string state) {
  if (!last_api_connected_) {
    ESP_LOGV(TAG, "Ignoring state '%s' (API disconnected)", state.c_str());
    return;
  }
  last_entity_state_mapped_ = map_alarm_state_(state);
  maybe_publish_updated_state_();
}

// ---------- Mapping & Inference ----------
std::string K1AlarmListener::map_alarm_state_(const std::string &raw_in) const {
  std::string raw = lower_copy_(trim_copy_(raw_in));
  if (raw == "unavailable" || raw == "unknown")
    return CONNECTION_TIMEOUT_STATE;
  return raw;
}

bool K1AlarmListener::is_truthy_attr_list_(const std::string &v) const {
  if (v.empty()) return false;
  std::string low = lower_copy_(trim_copy_(v));
  if (low == "null" || low == "none" || low == "[]" || low == "{}") return false;
  return true;
}

std::string K1AlarmListener::compute_inferred_state_() const {
  // Pre: last_entity_state_mapped_ already mapped (connection/unavailable logic)
  const std::string &base = last_entity_state_mapped_;

  if (base.empty()) return "";
  if (base == CONNECTION_TIMEOUT_STATE) return CONNECTION_TIMEOUT_STATE;

  if (!attributes_supported_) {
    return base;  // cannot enhance
  }

  std::string arm_mode = lower_copy_(trim_copy_(attr_.arm_mode));
  std::string next_state = lower_copy_(trim_copy_(attr_.next_state));
  bool bypassed_non_empty = is_truthy_attr_list_(attr_.bypassed_sensors);

  if (base == "disarmed") return "disarmed";
  if (base == "triggered") return "triggered";

  if (base == "arming") {
    if (arm_mode == "armed_home") return "arming_home";
    if (arm_mode == "armed_away") return "arming_away";
    return "arming";
  }

  if (base == "pending") {
    if (arm_mode == "armed_home" || next_state == "armed_home") return "pending_home";
    if (arm_mode == "armed_away" || next_state == "armed_away") return "pending_away";
    return "pending";
  }

  if (base == "armed_home") {
    if (bypassed_non_empty) return "armed_home_bypass";
    return "armed_home";
  }

  if (base == "armed_away") {
    if (bypassed_non_empty) return "armed_away_bypass";
    return "armed_away";
  }

  // Extend later for night/vacation/custom inference if desired
  return base;
}

void K1AlarmListener::maybe_publish_updated_state_() {
  if (!main_sensor_) return;

  // If we don't have a mapped base yet, nothing to do
  if (last_entity_state_mapped_.empty()) return;

  std::string final_state = compute_inferred_state_();
  if (final_state.empty())
    return;

  // If API disconnected, override
  if (!last_api_connected_) {
    final_state = CONNECTION_TIMEOUT_STATE;
  }

  publish_state_if_changed_(final_state);
}

// ---------- Publishing ----------
void K1AlarmListener::publish_state_if_changed_(const std::string &st) {
  if (!main_sensor_) return;
  if (st == last_published_state_) {
    ESP_LOGV(TAG, "State unchanged (%s)", st.c_str());
    return;
  }
  ESP_LOGD(TAG, "Publishing state: %s", st.c_str());
  main_sensor_->publish_state(st);
  last_published_state_ = st;
}

// ---------- String Utilities ----------
std::string K1AlarmListener::lower_copy_(const std::string &s) {
  std::string out;
  out.reserve(s.size());
  for (char c : s) out.push_back((char) std::tolower((unsigned char)c));
  return out;
}
std::string K1AlarmListener::trim_copy_(const std::string &s) {
  size_t a = 0, b = s.size();
  while (a < b && std::isspace((unsigned char)s[a])) ++a;
  while (b > a && std::isspace((unsigned char)s[b-1])) --b;
  return s.substr(a, b - a);
}

// ---------- Dump Config ----------
void K1AlarmListener::dump_config() {
  ESP_LOGCONFIG(TAG, "K1 Alarm Listener:");
  ESP_LOGCONFIG(TAG, "  Alarm Entity: %s", alarm_entity_.c_str());
  ESP_LOGCONFIG(TAG, "  Subscription: %s", subscription_started_ ? "started" : "not started");
  ESP_LOGCONFIG(TAG, "  API Connected (last): %s", last_api_connected_ ? "YES" : "NO");
  ESP_LOGCONFIG(TAG, "  Attributes supported: %s", attributes_supported_ ? "YES" : "NO");
  ESP_LOGCONFIG(TAG, "  Mapping: unavailable|unknown -> %s", CONNECTION_TIMEOUT_STATE);
  if (!last_published_state_.empty())
    ESP_LOGCONFIG(TAG, "  Last Published: %s", last_published_state_.c_str());
}

}  // namespace k1_alarm_listener
}  // namespace esphome
