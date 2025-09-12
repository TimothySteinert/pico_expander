#include "k1_alarm_listener.h"
#include "esphome/core/log.h"
#include "esp_timer.h"

namespace esphome {
namespace k1_alarm_listener {

static const char *const TAG = "k1_alarm_listener.core";
static inline uint32_t now_ms() { return (uint32_t)(esp_timer_get_time()/1000ULL); }

void K1AlarmListener::register_ha_connection_sensor(binary_sensor::BinarySensor *bs) {
  ha_connection_sensor_ = bs;
  update_connection_sensor_(last_api_connected_);
}

void K1AlarmListener::setup() {
  state_main_.init(this, &handling_);
  api_.init(this, &handling_);

  if (!initial_placeholder_done_) {
    state_main_.publish_initial_placeholder();
    initial_placeholder_done_ = true;
  }

  if (alarm_entity_.empty()) {
    ESP_LOGE(TAG, "No alarm_entity configured!");
    return;
  }

  this->subscribe_homeassistant_state(&K1AlarmListener::ha_state_callback_, alarm_entity_);
  subscription_started_ = true;
  ESP_LOGI(TAG, "Subscribed to state of '%s'", alarm_entity_.c_str());

  this->subscribe_homeassistant_state(&K1AlarmListener::arm_mode_attr_callback_, alarm_entity_, "arm_mode");
  this->subscribe_homeassistant_state(&K1AlarmListener::next_state_attr_callback_, alarm_entity_, "next_state");
  this->subscribe_homeassistant_state(&K1AlarmListener::bypassed_attr_callback_, alarm_entity_, "bypassed_sensors");

  this->register_service(&K1AlarmListener::failed_arm_service_, "failed_arm", {"reason"});

  last_api_connected_ = api_connected_now_();
  update_connection_sensor_(last_api_connected_);
  handling_.set_connection(last_api_connected_);
  state_main_.schedule_publish();
}

void K1AlarmListener::loop() {
  bool connected = api_connected_now_();
  if (connected != last_api_connected_) {
    ESP_LOGI(TAG, "API connection %s", connected ? "RESTORED" : "LOST");
    last_api_connected_ = connected;
    update_connection_sensor_(connected);
    handling_.set_connection(connected);
    state_main_.schedule_publish();
  }
  state_main_.on_loop(now_ms());
}

void K1AlarmListener::dump_config() {
  ESP_LOGCONFIG(TAG, "K1 Alarm Listener:");
  ESP_LOGCONFIG(TAG, "  Alarm Entity: %s", alarm_entity_.c_str());
  ESP_LOGCONFIG(TAG, "  Incorrect PIN Timeout: %u ms", (unsigned) incorrect_pin_timeout_ms_);
  ESP_LOGCONFIG(TAG, "  Failed Open Sensors Timeout: %u ms", (unsigned) failed_open_sensors_timeout_ms_);
  ESP_LOGCONFIG(TAG, "  Subscriptions started: %s", subscription_started_ ? "YES":"NO");
  ESP_LOGCONFIG(TAG, "  API Connected (last): %s", last_api_connected_ ? "YES":"NO");
  ESP_LOGCONFIG(TAG, "  Override Active: %s", handling_.override_active() ? "YES":"NO");
  if (handling_.override_active())
    ESP_LOGCONFIG(TAG, "    Override State: %s", handling_.override_state().c_str());
  ESP_LOGCONFIG(TAG, "  Last Published: %s", state_main_.last_published().c_str());
}

bool K1AlarmListener::api_connected_now_() const {
#ifdef USE_API
  if (api::global_api_server == nullptr) return false;
  return api::global_api_server->is_connected();
#else
  return true;
#endif
}

void K1AlarmListener::update_connection_sensor_(bool connected) {
  if (ha_connection_sensor_)
    ha_connection_sensor_->publish_state(connected);
}

void K1AlarmListener::ha_state_callback_(std::string s) { api_.ha_state_callback(std::move(s)); }
void K1AlarmListener::arm_mode_attr_callback_(std::string v) { api_.attr_arm_mode_callback(std::move(v)); }
void K1AlarmListener::next_state_attr_callback_(std::string v) { api_.attr_next_state_callback(std::move(v)); }
void K1AlarmListener::bypassed_attr_callback_(std::string v) { api_.attr_bypassed_callback(std::move(v)); }
void K1AlarmListener::failed_arm_service_(std::string reason) { api_.failed_arm_service(std::move(reason)); }

}  // namespace k1_alarm_listener
}  // namespace esphome
