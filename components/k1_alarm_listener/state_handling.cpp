#include "state_handling.h"
#include <algorithm>

namespace esphome {
namespace k1_alarm_listener {

static inline std::string lower_copy(const std::string &s) {
  std::string o; o.reserve(s.size());
  for (char c: s) o.push_back((char) std::tolower((unsigned char)c));
  return o;
}
static inline std::string trim_copy(const std::string &s) {
  size_t a=0,b=s.size();
  while (a<b && std::isspace((unsigned char)s[a])) ++a;
  while (b>a && std::isspace((unsigned char)s[b-1])) --b;
  return s.substr(a,b-a);
}

void K1AlarmStateHandling::set_connection(bool connected) {
  if (api_connected_ == connected) return;
  api_connected_ = connected;
  if (!api_connected_) {
    clear_override_();
  }
  mark_dirty_();
}

void K1AlarmStateHandling::on_raw_state(const std::string &raw_state) {
  std::string mapped = map_raw_state_(raw_state);
  if (mapped != base_state_mapped_) {
    base_state_mapped_ = mapped;
    waiting_for_transitional_attrs_ = false;
    publish_retry_count_ = 0;
    mark_dirty_();
  }
}

void K1AlarmStateHandling::on_attr_arm_mode(const std::string &v) {
  attr_.arm_mode = trim_copy(v);
  bool before = attr_.arm_mode_seen;
  attr_.arm_mode_seen = attr_has_value_(attr_.arm_mode);
  if (attr_.arm_mode_seen && !before) attributes_supported_ = true;
  if (is_transitional_(base_state_mapped_)) mark_dirty_();
}

void K1AlarmStateHandling::on_attr_next_state(const std::string &v) {
  attr_.next_state = trim_copy(v);
  bool before = attr_.next_state_seen;
  attr_.next_state_seen = attr_has_value_(attr_.next_state);
  if (attr_.next_state_seen && !before) attributes_supported_ = true;
  if (is_transitional_(base_state_mapped_)) mark_dirty_();
}

void K1AlarmStateHandling::on_attr_bypassed(const std::string &v) {
  attr_.bypassed = trim_copy(v);
  bool before = attr_.bypassed_seen;
  attr_.bypassed_seen = attr_has_value_(attr_.bypassed);
  if (attr_.bypassed_seen && !before) attributes_supported_ = true;
  mark_dirty_(); // may upgrade armed_* to *_bypass
}

void K1AlarmStateHandling::on_failed_arm_reason(const std::string &reason) {
  if (!api_connected_) return;
  if (base_state_mapped_.empty() || base_state_mapped_ == "connection_timeout") return;
  std::string r = lower_copy(trim_copy(reason));
  uint32_t now_ms = 0;
  if (r == "invalid_code" || r == "not_allowed") {
    start_override_("incorrect_pin", incorrect_pin_timeout_ms_, now_ms);
  } else if (r == "open_sensors") {
    start_override_("failed_open_sensors", failed_open_sensors_timeout_ms_, now_ms);
  }
}

void K1AlarmStateHandling::tick(uint32_t now_ms) {
  if (override_active_ && now_ms >= override_end_ms_) {
    clear_override_();
    mark_dirty_();
  }
}

std::string K1AlarmStateHandling::current_effective_state(uint32_t /*now_ms*/) {
  if (!api_connected_ || base_state_mapped_ == "connection_timeout" || base_state_mapped_.empty())
    return "connection_timeout";

  if (is_transitional_(base_state_mapped_)) {
    if (!have_transitional_attrs_(base_state_mapped_)) {
      if (publish_retry_count_ >= MAX_PUBLISH_RETRIES) {
        return base_state_mapped_;
      } else {
        return "connection_timeout";
      }
    }
  }

  std::string underlying = compute_effective_without_override_();
  if (override_active_)
    return override_state_;
  return underlying;
}

// ----- Internal helpers -----
std::string K1AlarmStateHandling::map_raw_state_(const std::string &raw) const {
  std::string r = lower_copy(trim_copy(raw));
  if (r == "unavailable" || r == "unknown")
    return "connection_timeout";
  return r;
}

bool K1AlarmStateHandling::is_transitional_(const std::string &mapped) const {
  return mapped == "arming" || mapped == "pending";
}

bool K1AlarmStateHandling::have_transitional_attrs_(const std::string &mapped) const {
  if (mapped == "arming") return attr_.arm_mode_seen;
  if (mapped == "pending") return attr_.arm_mode_seen || attr_.next_state_seen;
  return true;
}

bool K1AlarmStateHandling::attr_has_value_(const std::string &v) const {
  if (v.empty()) return false;
  std::string low = lower_copy(v);
  return !(low == "null" || low == "none");
}

bool K1AlarmStateHandling::is_truthy_list_(const std::string &v) const {
  if (v.empty()) return false;
  std::string low = lower_copy(trim_copy(v));
  if (low == "null" || low == "none" || low == "[]" || low == "{}") return false;
  return true;
}

std::string K1AlarmStateHandling::compute_effective_without_override_() const {
  if (alarm_type_ == AlarmIntegrationType::OTHER)
    return base_state_mapped_;
  return infer_state_core_(base_state_mapped_);
}

std::string K1AlarmStateHandling::infer_state_core_(const std::string &mapped) const {
  if (alarm_type_ != AlarmIntegrationType::ALARMO) return mapped;
  if (mapped == "connection_timeout") return mapped;

  std::string arm_mode = lower_copy(trim_copy(attr_.arm_mode));
  std::string next_state = lower_copy(trim_copy(attr_.next_state));
  bool bypassed_non_empty = is_truthy_list_(attr_.bypassed);

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

void K1AlarmStateHandling::start_override_(const std::string &st, uint32_t dur_ms, uint32_t now_ms) {
  override_active_ = true;
  override_state_ = st;
  override_end_ms_ = now_ms ? (now_ms + dur_ms) : dur_ms;
  mark_dirty_();
}

void K1AlarmStateHandling::clear_override_() {
  if (!override_active_) return;
  override_active_ = false;
  override_state_.clear();
  override_end_ms_ = 0;
  mark_dirty_();
}

} // namespace k1_alarm_listener
} // namespace esphome
