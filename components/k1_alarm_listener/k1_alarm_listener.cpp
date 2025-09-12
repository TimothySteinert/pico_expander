#include "k1_alarm_listener.h"
#include "esphome/core/log.h"

namespace esphome {
namespace k1_alarm_listener {

static const char *const TAG = "k1_alarm_listener";

// ---------- Text sensor registration ----------
void K1AlarmListener::set_text_sensor(K1AlarmListenerTextSensor *ts) {
  main_sensor_ = ts;
  if (!initial_state_published_) {
    publish_initial_connection_timeout_();
  } else if (!last_published_state_.empty()) {
    publish_state_if_changed_(last_published_state_);
  }
}

// ---------- Setup ----------
void K1AlarmListener::setup() {
  if (alarm_entity_.empty()) {
    ESP_LOGE(TAG, "No alarm_entity configured.");
    return;
  }
  publish_initial_connection_timeout_();

  this->subscribe_homeassistant_state(&K1AlarmListener::ha_state_callback_, this->alarm_entity_);
  subscription_started_ = true;
  ESP_LOGI(TAG, "Subscribed to '%s' (type=%s)", alarm_entity_.c_str(),
           alarm_type_ == AlarmIntegrationType::ALARMO ? "alarmo" : "other");

  if (alarm_type_ == AlarmIntegrationType::ALARMO) {
    this->subscribe_homeassistant_state(&K1AlarmListener::arm_mode_attr_callback_, this->alarm_entity_, "arm_mode");
    this->subscribe_homeassistant_state(&K1AlarmListener::next_state_attr_callback_, this->alarm_entity_, "next_state");
    this->subscribe_homeassistant_state(&K1AlarmListener::bypassed_attr_callback_, this->alarm_entity_, "bypassed_sensors");
    ESP_LOGI(TAG, "Attribute subscriptions enabled");
  }

  last_api_connected_ = api_connected_now_();
  update_connection_sensor_(last_api_connected_);
  if (!last_api_connected_)
    ESP_LOGD(TAG, "API not connected yet; holding connection_timeout");
}

void K1AlarmListener::loop() {
  bool connected = api_connected_now_();
  if (connected != last_api_connected_) {
    ESP_LOGI(TAG, "API connection %s", connected ? "RESTORED" : "LOST");
    last_api_connected_ = connected;
    update_connection_sensor_(connected);
    if (!connected) {
      publish_state_if_changed_(CONNECTION_TIMEOUT_STATE);
      waiting_for_transitional_attrs_ = false;
    } else {
      publish_state_if_changed_(CONNECTION_TIMEOUT_STATE);
      waiting_for_transitional_attrs_ = false;
    }
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
    waiting_for_transitional_attrs_ = false;
  }
}

// ---------- Attribute callbacks ----------
bool K1AlarmListener::attr_has_value_(const std::string &v) {
  if (v.empty()) return false;
  std::string low;
  low.reserve(v.size());
  for (char c: v) low.push_back((char) std::tolower((unsigned char)c));
  return !(low == "null" || low == "none");
}

void K1AlarmListener::arm_mode_attr_callback_(std::string value) {
  if (alarm_type_ != AlarmIntegrationType::ALARMO) return;
  attr_.arm_mode = trim_copy_(value);
  bool before = attr_.arm_mode_seen;
  attr_.arm_mode_seen = attr_has_value_(attr_.arm_mode);
  if (attr_.arm_mode_seen && !before) attributes_supported_ = true;
  schedule_publish_();
}

void K1AlarmListener::next_state_attr_callback_(std::string value) {
  if (alarm_type_ != AlarmIntegrationType::ALARMO) return;
  attr_.next_state = trim_copy_(value);
  bool before = attr_.next_state_seen;
  attr_.next_state_seen = attr_has_value_(attr_.next_state);
  if (attr_.next_state_seen && !before) attributes_supported_ = true;
  schedule_publish_();
}

void K1AlarmListener::bypassed_attr_callback_(std::string value) {
  if (alarm_type_ != AlarmIntegrationType::ALARMO) return;
  attr_.bypassed_sensors = trim_copy_(value);
  bool before = attr_.bypassed_seen;
  attr_.bypassed_seen = attr_has_value_(attr_.bypassed_sensors);
  if (attr_.bypassed_seen && !before) attributes_supported_ = true;
  schedule_publish_();
}

// ---------- State callback ----------
void K1AlarmListener::ha_state_callback_(std::string state) {
  if (!last_api_connected_) {
    ESP_LOGV(TAG, "Ignoring state '%s' (API disconnected)", state.c_str());
    return;
  }
  current_base_state_ = map_alarm_state_(state);
  schedule_publish_();
}

// ---------- Coalesced publish control ----------
void K1AlarmListener::schedule_publish_() {
  pending_publish_ = true;
  if (publish_scheduled_) return;
  publish_scheduled_ = true;
  // Defer to end-of-cycle to aggregate all attributes & state
  this->set_timeout(0, [this]() { this->finalize_publish_(); });
}

void K1AlarmListener::finalize_publish_() {
  publish_scheduled_ = false;
  if (!pending_publish_) return;
  pending_publish_ = false;

  // Always honor connection timeout
  if (!last_api_connected_) {
    publish_state_if_changed_(CONNECTION_TIMEOUT_STATE);
    return;
  }
  if (current_base_state_.empty()) {
    // Nothing yet (still awaiting first state)
    publish_state_if_changed_(CONNECTION_TIMEOUT_STATE);
    return;
  }
  if (current_base_state_ == CONNECTION_TIMEOUT_STATE) {
    publish_state_if_changed_(CONNECTION_TIMEOUT_STATE);
    return;
  }

  // OTHER integration: straight pass-through
  if (alarm_type_ == AlarmIntegrationType::OTHER) {
    publish_state_if_changed_(current_base_state_);
    return;
  }

  // ALARMO path
  bool transitional = is_transitional_(current_base_state_);
  if (transitional) {
    bool have = have_transitional_attrs_(current_base_state_);
    if (!have) {
      waiting_for_transitional_attrs_ = true;
      if (publish_retry_count_ < MAX_PUBLISH_RETRIES) {
        publish_retry_count_++;
        // Retry next loop – still show connection_timeout (placeholder) – no flicker
        schedule_publish_();
        return;
      } else {
        // Give up – publish generic transitional state (rare fallback)
        publish_state_if_changed_(current_base_state_);
        waiting_for_transitional_attrs_ = false;
        publish_retry_count_ = 0;
        return;
      }
    }
    // Got required attributes
    waiting_for_transitional_attrs_ = false;
    publish_retry_count_ = 0;
  } else {
    // For armed_* states we publish immediately (no wait for bypass)
    waiting_for_transitional_attrs_ = false;
    publish_retry_count_ = 0;
  }

  std::string inferred = infer_state_(current_base_state_);
  last_inferred_state_ = inferred;
  publish_state_if_changed_(inferred);
}

// ---------- Mapping & inference helpers ----------
std::string K1AlarmListener::map_alarm_state_(const std::string &raw_in) const {
  std::string raw = lower_copy_(trim_copy_(raw_in));
  if (raw == "unavailable" || raw == "unknown")
    return CONNECTION_TIMEOUT_STATE;
  return raw;
}

bool K1AlarmListener::is_transitional_(const std::string &mapped) const {
  return (mapped == "arming" || mapped == "pending");
}
bool K1AlarmListener::have_transitional_attrs_(const std::string &mapped) const {
  if (mapped == "arming")  return attr_.arm_mode_seen;
  if (mapped == "pending") return attr_.arm_mode_seen || attr_.next_state_seen;
  return true;
}

bool K1AlarmListener::is_truthy_list_(const std::string &v) const {
  if (v.empty()) return false;
  std::string low = lower_copy_(trim_copy_(v));
  if (low == "null" || low == "none" || low == "[]" || low == "{}") return false;
  return true;
}

std::string K1AlarmListener::infer_state_(const std::string &mapped) const {
  if (alarm_type_ != AlarmIntegrationType::ALARMO)
    return mapped;
  if (mapped == CONNECTION_TIMEOUT_STATE) return mapped;

  std::string arm_mode = lower_copy_(trim_copy_(attr_.arm_mode));
  std::string next_state = lower_copy_(trim_copy_(attr_.next_state));
  bool bypassed_non_empty = is_truthy_list_(attr_.bypassed_sensors);

  // Transitional
  if (mapped == "arming") {
    if (arm_mode == "armed_home") return "arming_home";
    if (arm_mode == "armed_away") return "arming_away";
    if (arm_mode == "armed_night") return "arming_night";
    if (arm_mode == "armed_vacation") return "arming_vacation";
    if (arm_mode == "armed_custom_bypass") return "arming_custom";
    return mapped;  // fallback if unknown mode
  }
  if (mapped == "pending") {
    if (arm_mode == "armed_home"        || next_state == "armed_home")        return "pending_home";
    if (arm_mode == "armed_away"        || next_state == "armed_away")        return "pending_away";
    if (arm_mode == "armed_night"       || next_state == "armed_night")       return "pending_night";
    if (arm_mode == "armed_vacation"    || next_state == "armed_vacation")    return "pending_vacation";
    if (arm_mode == "armed_custom_bypass" || next_state == "armed_custom_bypass") return "pending_custom";
    return mapped;
  }

  // Armed steady states – publish immediately; upgrade later if bypass attr appears
  if (mapped == "armed_home")       return bypassed_non_empty ? "armed_home_bypass" : "armed_home";
  if (mapped == "armed_away")       return bypassed_non_empty ? "armed_away_bypass" : "armed_away";
  if (mapped == "armed_night")      return bypassed_non_empty ? "armed_night_bypass" : "armed_night";
  if (mapped == "armed_vacation")   return bypassed_non_empty ? "armed_vacation_bypass" : "armed_vacation";
  if (mapped == "armed_custom_bypass") {
    // Alarmo uses armed_custom_bypass as the base for custom force; normalize
    return bypassed_non_empty ? "armed_custom_bypass" : "armed_custom";
  }

  return mapped;
}

// ---------- Initial publish ----------
void K1AlarmListener::publish_initial_connection_timeout_() {
  if (initial_state_published_) return;
  publish_state_if_changed_(CONNECTION_TIMEOUT_STATE);
  initial_state_published_ = true;
}

// ---------- Publish ----------
void K1AlarmListener::publish_state_if_changed_(const std::string &st) {
  if (!main_sensor_) {
    last_published_state_ = st;
    return;
  }
  if (!last_api_connected_ && st != CONNECTION_TIMEOUT_STATE) return;
  if (st == last_published_state_) {
    ESP_LOGV(TAG, "State unchanged (%s)", st.c_str());
    return;
  }
  ESP_LOGD(TAG, "Publishing state: %s", st.c_str());
  main_sensor_->publish_state(st);
  last_published_state_ = st;
}

// ---------- Utils ----------
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
  ESP_LOGCONFIG(TAG, "  Alarm Type: %s", alarm_type_ == AlarmIntegrationType::ALARMO ? "alarmo" : "other");
  ESP_LOGCONFIG(TAG, "  Subscription: %s", subscription_started_ ? "started" : "not started");
  ESP_LOGCONFIG(TAG, "  API Connected (last): %s", last_api_connected_ ? "YES" : "NO");
  if (alarm_type_ == AlarmIntegrationType::ALARMO) {
    ESP_LOGCONFIG(TAG, "  Attributes seen: arm_mode=%s next_state=%s bypassed=%s",
                  attr_.arm_mode_seen ? "Y":"N",
                  attr_.next_state_seen ? "Y":"N",
                  attr_.bypassed_seen ? "Y":"N");
    ESP_LOGCONFIG(TAG, "  Waiting transitional attrs: %s", waiting_for_transitional_attrs_ ? "YES" : "NO");
    ESP_LOGCONFIG(TAG, "  Publish retry count: %u", publish_retry_count_);
  }
  if (!last_published_state_.empty())
    ESP_LOGCONFIG(TAG, "  Last Published: %s", last_published_state_.c_str());
  ESP_LOGCONFIG(TAG, "  Mapping: unavailable|unknown -> %s", CONNECTION_TIMEOUT_STATE);
}

}  // namespace k1_alarm_listener
}  // namespace esphome
