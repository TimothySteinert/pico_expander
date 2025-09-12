#pragma once
#include <string>
#include "k1_alarm_listener_state_handling.h"

namespace esphome {
namespace k1_alarm_listener {

class K1AlarmListener;
class K1AlarmListenerTextSensor;
class K1AlarmListenerBinarySensor;

// Outward-facing publisher/coalescer
class K1AlarmStateMain {
 public:
  void init(K1AlarmListener *parent, K1AlarmStateHandling *handling) {
    parent_ = parent; handling_ = handling;
  }

  void attach_text_sensor(K1AlarmListenerTextSensor *ts);
  void attach_connection_sensor(K1AlarmListenerBinarySensor *bs);

  void schedule_publish();
  void finalize_publish();
  void on_loop(uint32_t now_ms);

  void publish_initial_placeholder();

  const std::string &last_published() const { return last_published_state_; }

 private:
  K1AlarmListener *parent_{nullptr};
  K1AlarmStateHandling *handling_{nullptr};

  K1AlarmListenerTextSensor *text_sensor_{nullptr};
  K1AlarmListenerBinarySensor *connection_sensor_{nullptr};

  bool publish_scheduled_{false};
  bool pending_publish_{false};

  std::string last_published_state_;

  void publish_state_if_changed_(const std::string &st);
};

}  // namespace k1_alarm_listener
}  // namespace esphome
