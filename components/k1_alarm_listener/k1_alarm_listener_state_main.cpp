#include "k1_alarm_listener_state_main.h"
#include "k1_alarm_listener.h"
#include "esphome/core/log.h"
#include "esp_timer.h"

namespace esphome {
namespace k1_alarm_listener {

static const char *const TAG_MAIN = "k1_alarm_listener.main";
static inline uint32_t now_ms() { return (uint32_t)(esp_timer_get_time()/1000ULL); }

void K1AlarmStateMain::attach_text_sensor(K1AlarmListenerTextSensor *ts) {
  text_sensor_ = ts;
  if (!last_published_state_.empty())
    publish_state_if_changed_(last_published_state_);
}

void K1AlarmStateMain::attach_connection_sensor(K1AlarmListenerBinarySensor *bs) {
  connection_sensor_ = bs;
}

void K1AlarmStateMain::schedule_publish() {
  pending_publish_ = true;
  if (publish_scheduled_ || parent_ == nullptr) return;
  publish_scheduled_ = true;
  parent_->set_timeout(0, [this]() {
    this->publish_scheduled_ = false;
    this->finalize_publish();
  });
}

void K1AlarmStateMain::finalize_publish() {
  if (!pending_publish_) return;
  pending_publish_ = false;
  if (!handling_) return;

  std::string effective = handling_->current_effective_state(now_ms());
  handling_->clear_publish_flag();
  publish_state_if_changed_(effective);
}

void K1AlarmStateMain::publish_initial_placeholder() {
  publish_state_if_changed_("connection_timeout");
}

void K1AlarmStateMain::on_loop(uint32_t now) {
  if (!handling_) return;
  handling_->tick(now);
  if (handling_->needs_publish())
    schedule_publish();
}

void K1AlarmStateMain::publish_state_if_changed_(const std::string &st) {
  if (st == last_published_state_) return;
  ESP_LOGD(TAG_MAIN, "Publishing state: %s", st.c_str());
  last_published_state_ = st;
  if (text_sensor_) text_sensor_->publish_state(st);
}

}  // namespace k1_alarm_listener
}  // namespace esphome
