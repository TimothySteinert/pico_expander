#pragma once
#include <string>
#include "state_handling.h"

namespace esphome {
namespace k1_alarm_listener {

class K1AlarmListener;  // forward

// Handles subscription callbacks & custom service -> passes to logic layer
class K1AlarmApi {
 public:
  void init(K1AlarmListener *parent, K1AlarmStateHandling *handling) {
    parent_ = parent; handling_ = handling;
  }

  // Called by parent when connection toggles
  void on_connection_changed(bool connected);

  // Exposed so parent can register these as HA callbacks
  void ha_state_callback(std::string state);
  void attr_arm_mode_callback(std::string value);
  void attr_next_state_callback(std::string value);
  void attr_bypassed_callback(std::string value);

  // Service
  void failed_arm_service(std::string reason);

 private:
  K1AlarmListener *parent_{nullptr};
  K1AlarmStateHandling *handling_{nullptr};

  void mark_publish_needed_();
};

}  // namespace k1_alarm_listener
}  // namespace esphome
