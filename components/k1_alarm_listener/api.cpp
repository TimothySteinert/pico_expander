#include "api.h"
#include "k1_alarm_listener.h"

namespace esphome {
namespace k1_alarm_listener {

void K1AlarmApi::on_connection_changed(bool connected) {
  if (!handling_) return;
  handling_->set_connection(connected);
  mark_publish_needed_();
}

void K1AlarmApi::ha_state_callback(std::string state) {
  if (!handling_) return;
  handling_->on_raw_state(state);
  mark_publish_needed_();
}

void K1AlarmApi::attr_arm_mode_callback(std::string value) {
  if (!handling_) return;
  handling_->on_attr_arm_mode(value);
  mark_publish_needed_();
}

void K1AlarmApi::attr_next_state_callback(std::string value) {
  if (!handling_) return;
  handling_->on_attr_next_state(value);
  mark_publish_needed_();
}

void K1AlarmApi::attr_bypassed_callback(std::string value) {
  if (!handling_) return;
  handling_->on_attr_bypassed(value);
  mark_publish_needed_();
}

void K1AlarmApi::failed_arm_service(std::string reason) {
  if (!handling_) return;
  handling_->on_failed_arm_reason(reason);
  mark_publish_needed_();
}

void K1AlarmApi::mark_publish_needed_() {
  if (!parent_) return;
  parent_->schedule_state_publish();
}

}  // namespace k1_alarm_listener
}  // namespace esphome
