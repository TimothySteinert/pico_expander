#include "k1_alarm_listener.h"
#include "esphome/core/log.h"
#include <algorithm>
#include <cctype>

namespace esphome {
namespace k1_alarm_listener {

static const char *const TAG = "k1_alarm_listener";

// ---------- Setup ----------
void K1AlarmListener::setup() {
  if (alarm_entity_.empty()) {
    ESP_LOGE(TAG, "No alarm_entity configured.");
    return;
  }

  // Always subscribe to base state
  this->subscribe_homeassistant_state(&K1AlarmListener::ha_state_callback_, this->alarm_entity_);
  subscription_started_ = true;
  ESP_LOGI(TAG, "Subscribed to state of '%s' (type=%s)",
           alarm_entity_.c_str(),
           alarm_type_ == AlarmIntegrationType::ALARMO ? "alarmo" : "other");

  // Subscribe to attributes only for Alarmo
  if (alarm_type_ == AlarmIntegrationType::ALARMO) {
    this->subscribe_homeassistant_state(&K1AlarmListener::arm_mode_attr_callback_, this->alarm_entity_, "arm_mode");
    this->subscribe_homeassistant_state(&K1AlarmListener::next_state_attr_callback_, this->alarm_entity_, "next_state");
    this->subscribe_homeassistant_state(&K1AlarmListener::bypassed_attr_callback_, this->alarm_entity_, "bypassed_sensors");
    ESP_LOGI(TAG, "Attribute subscriptions enabled (arm_mode, next_state, bypassed_sensors)");
  }

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

// ---------- Connection ----------
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
    waiting_for_attributes_ = false;
  }
}

// ---------- Attribute Helpers ----------
bool K1AlarmListener::attr_has_value_(const std::string &v) {
  if (v.empty()) return false;
  std::string low;
  low.reserve(v.size());
  for (char c : v) low.push_back((char) std::tolower((unsigned char)c));
  return !(low == "null" || low == "none");
}

void K1AlarmListener::arm_mode_attr_callback_(std::string value) {
  if (alarm_type_ != AlarmIntegrationType::ALARMO) return;
  attr_.arm_mode = trim_copy_(value);
  bool before = attr_.arm_mode_seen;
  attr_.arm_mode_seen = attr_has_value_(attr_.arm_mode);
  if (attr_.arm_mode_seen && !before) attributes_supported_ = true;
  if (waiting_for_attributes_) attempt_publish_alarmo_();
}

void K1AlarmListener::next_state_attr_callback_(std::string value) {
  if (alarm_type_ != AlarmIntegrationType::ALARMO) return;
  attr_.next_state = trim_copy_(value);
  bool before = attr_.next_state_seen;
  attr_.next_state_seen = attr_has_value_(attr_.next_state);
  if (attr_.next_state_seen && !before) attributes_supported_ = true;
  if (waiting_for_attributes_) attempt_publish_alarmo_();
}

void K1AlarmListener::bypassed_attr_callback_(std::string value) {
  if (alarm_type_ != AlarmIntegrationType::ALARMO) return;
  attr_.bypassed_sensors = trim_copy_(value);
  bool before = attr_.bypassed_seen;
  attr_.bypassed_seen = attr_has_value_(attr_.bypassed_sensors);
  if (attr_.bypassed_seen && !before) attributes_supported_ = true;
  if (waiting_for_attributes_) attempt_publish_alarmo_();
}

// ---------- State Callback ----------
void K1AlarmListener::ha_state_callback_(std::string state) {
  if (!last_api_connected_) {
    ESP_LOGV(TAG, "Ignoring state '%s' (API disconnected)", state.c_str());
    return;
  }
  current_base_state_ = map_alarm_state_(state);

  if (current_base_state_ == CONNECTION_TIMEOUT_STATE) {
    waiting_for_attributes_ = false;
    publish_state_if_changed_(current_base_state_);
    return;
  }

  if (alarm_type_ == AlarmIntegrationType::OTHER) {
    // Immediate publish (raw mapped)
    publish_state_if_changed_(current_base_state_);
    return;
  }

  // Alarmo path
  if (state_requires_attributes_(current_base_state_)) {
    // Decide if we already have what we need
    if (have_required_attributes_for_state_(current_base_state_)) {
      waiting_for_attributes_ = false;
      attempt_publish_alarmo_();
    } else {
      waiting_for_attributes_ = true;
      ESP_LOGD(TAG, "Holding state '%s' until attributes arrive.", current_base_state_.c_str());
    }
  } else {
    // No attributes needed for this state
    waiting_for_attributes_ = false;
    publish_state_if_changed_(compute_inferred_state_(current_base_state_));
  }
}

// ---------- Requirements ----------
bool K1AlarmListener::state_requires_attributes_(const std::string &mapped) const {
  if (mapped == "arming" || mapped == "pending" ||
      mapped == "armed_home" || mapped == "armed_away") {
    return true;
  }
  return false;
}

bool K1AlarmListener::have_required_attributes_for_state_(const std::string &mapped) const {
  if (mapped == "arming") {
    return attr_.arm_mode_seen;
  }
  if (mapped == "pending") {
    return attr_.arm_mode_seen || attr_.next_state_seen;
  }
  if (mapped == "armed_home" || mapped == "armed_away") {
    // need bypassed_sensors to avoid flicker of base vs bypass variant
    return attr_.bypassed_seen;
  }
  return true;
}

// ---------- Attempt Publish (Alarmo) ----------
void K1AlarmListener::attempt_publish_alarmo_() {
  if (current_base_state_.empty()) return;
  if (!have_required_attributes_for_state_(current_base_state_)) {
    // Still waiting
    return;
  }
  std::string inferred = compute_inferred_state_(current_base_state_);
  publish_state_if_changed_(inferred);
  waiting_for_attributes_ = false;
}

// ---------- Mapping & Inference ----------
std::string K1AlarmListener::map_alarm_state_(const std::string &raw_in) const {
  std::string raw = lower_copy_(trim_copy_(raw_in));
  if (raw == "unavailable" || raw == "unknown")
    return CONNECTION_TIMEOUT_STATE;
  return raw;
}

std::string K1AlarmListener::compute_inferred_state_(const std::string &mapped) const {
  if (alarm_type_ != AlarmIntegrationType::ALARMO)
    return mapped;

  if (mapped == CONNECTION_TIMEOUT_STATE) return mapped;

  // If attributes never confirmed we still return base (but since we control publish timing,
  // this means for 'arming' etc. we only get here once attributes are available).
  std::string arm_mode = lower_copy_(trim_copy_(attr_.arm_mode));
  std::string next_state = lower_copy_(trim_copy_(attr_.next_state));
  bool bypassed_non_empty = is_truthy_attr_list_(attr_.bypassed_sensors);

  if (mapped == "arming") {
    if (arm_mode == "armed_home") return "arming_home";
    if (arm_mode == "armed_away") return "arming_away";
    return mapped;
  }
  if (mapped == "pending") {
    if (arm_mode == "armed_home" || next_state == "armed_home") return "pending_home";
    if (arm_mode == "armed_away" || next_state == "armed_away") return "pending_away";
    return mapped;
  }
  if (mapped == "armed_home") {
    if (bypassed_non_empty) return "armed_home_bypass";
    return mapped;
  }
  if (mapped == "armed_away") {
    if (bypassed_non_empty) return "armed_away_bypass";
    return mapped;
  }
  return mapped;
}

bool K1AlarmListener::is_truthy_attr_list_(const std::string &v) const {
  if (v.empty()) return false;
  std::string low = lower_copy_(trim_copy_(v));
  if (low == "null" || low == "none" || low == "[]" || low == "{}") return false;
  return true;
}

// ---------- Utilities ----------
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

// ---------- Publish ----------
void K1AlarmListener::publish_state_if_changed_(const std::string &st) {
  if (!main_sensor_) return;
  if (!last_api_connected_) {
    if (st != CONNECTION_TIMEOUT_STATE) return;
  }
  if (st == last_published_state_) {
    ESP_LOGV(TAG, "State unchanged (%s)", st.c_str());
    return;
  }
  ESP_LOGD(TAG, "Publishing state: %s", st.c_str());
  main_sensor_->publish_state(st);
  last_published_state_ = st;
}

// ---------- Dump Config ----------
void K1AlarmListener::dump_config() {
  ESP_LOGCONFIG(TAG, "K1 Alarm Listener:");
  ESP_LOGCONFIG(TAG, "  Alarm Entity: %s", alarm_entity_.c_str());
  ESP_LOGCONFIG(TAG, "  Alarm Type: %s", alarm_type_ == AlarmIntegrationType::ALARMO ? "alarmo" : "other");
  ESP_LOGCONFIG(TAG, "  Subscription: %s", subscription_started_ ? "started" : "not started");
  ESP_LOGCONFIG(TAG, "  API Connected (last): %s", last_api_connected_ ? "YES" : "NO");
  if (alarm_type_ == AlarmIntegrationType::ALARMO) {
    ESP_LOGCONFIG(TAG, "  Attributes supported (ever seen): %s", attributes_supported_ ? "YES" : "NO");
    ESP_LOGCONFIG(TAG, "  Waiting for attributes: %s", waiting_for_attributes_ ? "YES" : "NO");
  }
  if (!last_published_state_.empty())
    ESP_LOGCONFIG(TAG, "  Last Published: %s", last_published_state_.c_str());
  ESP_LOGCONFIG(TAG, "  Mapping: unavailable|unknown -> %s", CONNECTION_TIMEOUT_STATE);
}

}  // namespace k1_alarm_listener
}  // namespace esphome
