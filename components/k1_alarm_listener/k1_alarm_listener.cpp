#include "k1_alarm_listener.h"
#include "esp_timer.h"

namespace esphome {
namespace k1_alarm_listener {

static const char *const TAG = "k1_alarm_listener";

// ---------- Static helper ----------
uint32_t K1AlarmListener::now_ms_() {
  return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

// ---------- Setup ----------
void K1AlarmListener::setup() {
  if (!initial_placeholder_done_) {
    publish_initial_placeholder_();
    initial_placeholder_done_ = true;
  }

  if (alarm_entity_.empty()) {
    ESP_LOGE(TAG, "alarm_entity not configured!");
    this->mark_failed();
    return;
  }

  // Subscribe to main state and attributes
  this->subscribe_homeassistant_state(&K1AlarmListener::ha_state_callback_, alarm_entity_);
  this->subscribe_homeassistant_state(&K1AlarmListener::arm_mode_attr_callback_, alarm_entity_, "arm_mode");
  this->subscribe_homeassistant_state(&K1AlarmListener::next_state_attr_callback_, alarm_entity_, "next_state");
  this->subscribe_homeassistant_state(&K1AlarmListener::bypassed_attr_callback_, alarm_entity_, "bypassed_sensors");

  // Custom service
  this->register_service(&K1AlarmListener::failed_arm_service_, "failed_arm", {"reason"});

  last_api_connected_ = api_connected_now_();
  update_connection_sensor_(last_api_connected_);

  // Force initial recompute
  mapped_state_.clear();
  schedule_state_publish();

  ESP_LOGI(TAG, "Subscribed to '%s' and attributes", alarm_entity_.c_str());
}

// ---------- Loop ----------
void K1AlarmListener::loop() {
  bool connected = api_connected_now_();
  if (connected != last_api_connected_) {
    last_api_connected_ = connected;
    ESP_LOGI(TAG, "API connection %s", connected ? "RESTORED" : "LOST");
    update_connection_sensor_(connected);
    // Connection change impacts effective state
    schedule_state_publish();
  }

  tick_override_expiry_();
}

// ---------- Dump Config ----------
void K1AlarmListener::dump_config() {
  ESP_LOGCONFIG(TAG, "K1 Alarm Listener:");
  ESP_LOGCONFIG(TAG, "  Alarm Entity: %s", alarm_entity_.c_str());
  ESP_LOGCONFIG(TAG, "  Alarm Type: %u", (unsigned) alarm_type_);
  ESP_LOGCONFIG(TAG, "  Incorrect PIN Timeout: %u ms", (unsigned) incorrect_pin_timeout_ms_);
  ESP_LOGCONFIG(TAG, "  Failed Open Sensors Timeout: %u ms", (unsigned) failed_open_sensors_timeout_ms_);
  ESP_LOGCONFIG(TAG, "  API Connected (last): %s", last_api_connected_ ? "YES" : "NO");
  ESP_LOGCONFIG(TAG, "  Last Published: %s", last_published_state_.c_str());
  ESP_LOGCONFIG(TAG, "  Override Active: %s", override_active_ ? "YES" : "NO");
  if (override_active_) {
    ESP_LOGCONFIG(TAG, "    Override State: %s (ends at %u ms)", override_state_.c_str(), override_end_ms_);
  }
  ESP_LOGCONFIG(TAG, "  Attrs seen: arm_mode=%s next_state=%s bypassed=%s",
    attr_arm_mode_seen_ ? "YES":"NO",
    attr_next_state_seen_ ? "YES":"NO",
    attr_bypassed_seen_ ? "YES":"NO");
}

// ---------- Scheduling ----------
void K1AlarmListener::schedule_state_publish() {
  publish_pending_ = true;
  if (publish_scheduled_) return;
  publish_scheduled_ = true;
  // Defer to next loop (0ms timeout)
  this->set_timeout(0, [this]() {
    this->publish_scheduled_ = false;
    this->finalize_publish_();
  });
}

void K1AlarmListener::finalize_publish_() {
  if (!publish_pending_) return;
  publish_pending_ = false;
  std::string eff = compute_effective_state_();
  publish_state_(eff);
}

// ---------- Publishing Helpers ----------
void K1AlarmListener::publish_initial_placeholder_() {
  publish_state_("connection_timeout");
}

void K1AlarmListener::publish_state_(const std::string &state) {
  if (state == last_published_state_) return;
  last_published_state_ = state;
  ESP_LOGD(TAG, "Publishing state: %s", state.c_str());
  if (text_sensor_) text_sensor_->publish_state(state);
}

void K1AlarmListener::update_connection_sensor_(bool connected) {
  if (ha_connection_sensor_) ha_connection_sensor_->publish_state(connected);
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

// ---------- Override Handling ----------
void K1AlarmListener::start_override_(const std::string &state, uint32_t dur_ms) {
  override_active_ = true;
  override_state_ = state;
  // We store absolute end time in ms relative to boot
  override_end_ms_ = now_ms_() + dur_ms;
  schedule_state_publish();
}

void K1AlarmListener::clear_override_() {
  if (!override_active_) return;
  override_active_ = false;
  override_state_.clear();
  override_end_ms_ = 0;
  schedule_state_publish();
}

void K1AlarmListener::tick_override_expiry_() {
  if (!override_active_) return;
  uint32_t now = now_ms_();
  if (now >= override_end_ms_) {
    ESP_LOGD(TAG, "Override expired (%s)", override_state_.c_str());
    clear_override_();
  }
}

// ---------- Attribute helpers ----------
bool K1AlarmListener::attr_has_value_(const std::string &v) const {
  if (v.empty()) return false;
  std::string low;
  low.reserve(v.size());
  for (char c: v) low.push_back((char) std::tolower((unsigned char)c));
  return !(low == "null" || low == "none");
}

bool K1AlarmListener::is_truthy_list_(const std::string &v) const {
  if (v.empty()) return false;
  std::string trimmed = v;
  // quick trim
  while (!trimmed.empty() && isspace((unsigned char)trimmed.front())) trimmed.erase(trimmed.begin());
  while (!trimmed.empty() && isspace((unsigned char)trimmed.back())) trimmed.pop_back();
  std::string low;
  low.reserve(trimmed.size());
  for (char c: trimmed) low.push_back((char) std::tolower((unsigned char)c));
  if (low == "null" || low == "none" || low == "[]" || low == "{}")
    return false;
  return true;
}

// ---------- Mapping & Transitional ----------
std::string K1AlarmListener::map_raw_state_(const std::string &raw) const {
  std::string low;
  low.reserve(raw.size());
  for (char c: raw) low.push_back((char) std::tolower((unsigned char)c));
  if (low == "unavailable" || low == "unknown")
    return "connection_timeout";
  return low;
}

bool K1AlarmListener::is_transitional_generic_(const std::string &s) const {
  return s == "arming" || s == "pending";
}

bool K1AlarmListener::transitional_has_required_attrs_(const std::string &mapped) const {
  if (mapped == "arming") return attr_arm_mode_seen_;
  if (mapped == "pending") return attr_arm_mode_seen_ || attr_next_state_seen_;
  return true;
}

// ---------- Effective State Computation ----------
std::string K1AlarmListener::compute_effective_state_() {
  // Connection baseline
  if (!api_connected_now_())
    return "connection_timeout";

  mapped_state_ = map_raw_state_(raw_state_);

  if (mapped_state_.empty())
    return "connection_timeout";

  if (mapped_state_ == "connection_timeout")
    return "connection_timeout";

  // If override is active, check expiry externally and show override
  if (override_active_) {
    // (expiry tick handled in loop)
    return override_state_;
  }

  // Transitional generic waiting for attributes? Provide generic only if attributes available or fallback?
  if (is_transitional_generic_(mapped_state_)) {
    if (!transitional_has_required_attrs_(mapped_state_)) {
      // Still waiting for disambiguation; show placeholder
      return "connection_timeout";
    }
  }

  // Alarm type inference
  std::string inferred = (alarm_type_ == AlarmIntegrationType::ALARMO)
                           ? infer_alarm_state_(mapped_state_)
                           : mapped_state_;

  return inferred;
}

std::string K1AlarmListener::infer_alarm_state_(const std::string &mapped) {
  // Build lowercase trimmed for attributes
  auto lower_trim = [](std::string s) {
    // trim
    while (!s.empty() && isspace((unsigned char)s.front())) s.erase(s.begin());
    while (!s.empty() && isspace((unsigned char)s.back())) s.pop_back();
    std::string out; out.reserve(s.size());
    for (char c: s) out.push_back((char) std::tolower((unsigned char)c));
    return out;
  };

  std::string arm_mode = lower_trim(attr_arm_mode_);
  std::string next_state = lower_trim(attr_next_state_);
  bool bypassed_non_empty = is_truthy_list_(attr_bypassed_);

  if (mapped == "arming") {
    if (arm_mode == "armed_home") return "arming_home";
    if (arm_mode == "armed_away") return "arming_away";
    if (arm_mode == "armed_night") return "arming_night";
    if (arm_mode == "armed_vacation") return "arming_vacation";
    if (arm_mode == "armed_custom_bypass") return "arming_custom";
    return mapped;
  }

  if (mapped == "pending") {
    if (arm_mode == "armed_home" || next_state == "armed_home") return "pending_home";
    if (arm_mode == "armed_away" || next_state == "armed_away") return "pending_away";
    if (arm_mode == "armed_night" || next_state == "armed_night") return "pending_night";
    if (arm_mode == "armed_vacation" || next_state == "armed_vacation") return "pending_vacation";
    if (arm_mode == "armed_custom_bypass" || next_state == "armed_custom_bypass") return "pending_custom";
    return mapped;
  }

  if (mapped == "armed_home")     return bypassed_non_empty ? "armed_home_bypass" : "armed_home";
  if (mapped == "armed_away")     return bypassed_non_empty ? "armed_away_bypass" : "armed_away";
  if (mapped == "armed_night")    return bypassed_non_empty ? "armed_night_bypass" : "armed_night";
  if (mapped == "armed_vacation") return bypassed_non_empty ? "armed_vacation_bypass" : "armed_vacation";
  if (mapped == "armed_custom_bypass")
    return bypassed_non_empty ? "armed_custom_bypass" : "armed_custom";

  return mapped;
}

// ---------- Failed Arm (Override) ----------
void K1AlarmListener::handle_failed_arm_reason_(const std::string &reason) {
  std::string low;
  low.reserve(reason.size());
  for (char c: reason) low.push_back((char) std::tolower((unsigned char)c));
  if (low == "invalid_code" || low == "not_allowed") {
    start_override_("incorrect_pin", incorrect_pin_timeout_ms_);
  } else if (low == "open_sensors") {
    start_override_("failed_open_sensors", failed_open_sensors_timeout_ms_);
  } else {
    ESP_LOGD(TAG, "Unknown failed_arm reason: %s", reason.c_str());
  }
}

// ---------- HA Callbacks ----------
void K1AlarmListener::ha_state_callback_(std::string s) {
  raw_state_ = std::move(s);
  schedule_state_publish();
}

void K1AlarmListener::arm_mode_attr_callback_(std::string v) {
  attr_arm_mode_ = std::move(v);
  bool prev = attr_arm_mode_seen_;
  attr_arm_mode_seen_ = attr_has_value_(attr_arm_mode_);
  if (attr_arm_mode_seen_ && !prev) attributes_supported_ = true;
  schedule_state_publish();
}

void K1AlarmListener::next_state_attr_callback_(std::string v) {
  attr_next_state_ = std::move(v);
  bool prev = attr_next_state_seen_;
  attr_next_state_seen_ = attr_has_value_(attr_next_state_);
  if (attr_next_state_seen_ && !prev) attributes_supported_ = true;
  schedule_state_publish();
}

void K1AlarmListener::bypassed_attr_callback_(std::string v) {
  attr_bypassed_ = std::move(v);
  bool prev = attr_bypassed_seen_;
  attr_bypassed_seen_ = attr_has_value_(attr_bypassed_);
  if (attr_bypassed_seen_ && !prev) attributes_supported_ = true;
  schedule_state_publish();
}

void K1AlarmListener::failed_arm_service_(std::string reason) {
  if (!api_connected_now_()) return;
  if (mapped_state_ == "connection_timeout" || mapped_state_.empty()) return;
  handle_failed_arm_reason_(reason);
}

}  // namespace k1_alarm_listener
}  // namespace esphome
