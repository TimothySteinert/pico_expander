#include "buzzer.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace buzzer {

static const char *const TAG = "buzzer";

void BuzzerComponent::loop() {
  if (!running_) return;

  uint32_t now = esphome::millis();
  if (now - last_tick_ < current_interval_) return;
  last_tick_ = now;

  if (beep_on_) {
    // Turn OFF
    apply_value_(0);
    beep_on_ = false;
    beeps_done_++;
    if (beeps_done_ >= beeps_) {
      if (repeat_) {
        beeps_done_ = 0;
        current_interval_ = long_pause_;
      } else {
        running_ = false;
      }
    } else {
      current_interval_ = short_pause_;
    }
  } else {
    // Turn ON
    apply_value_(tone_);
    beep_on_ = true;
    current_interval_ = beep_length_;
  }
}

void BuzzerComponent::start(uint8_t beeps, uint32_t short_pause, uint32_t long_pause,
                            uint8_t tone, bool repeat, uint32_t beep_length) {
  this->beeps_ = beeps;
  this->short_pause_ = short_pause;
  this->long_pause_ = long_pause;
  this->tone_ = tone;
  this->repeat_ = repeat;
  this->beep_length_ = beep_length;
  this->beeps_done_ = 0;
  this->beep_on_ = false;
  this->running_ = true;
  this->last_tick_ = esphome::millis();
  this->current_interval_ = 0;

  ESP_LOGD(TAG, "Started buzzer: beeps=%d short=%dms long=%dms tone=%d repeat=%d len=%dms",
           beeps, short_pause, long_pause, tone, repeat, beep_length);
}

void BuzzerComponent::stop() {
  this->running_ = false;
  apply_value_(0);
  ESP_LOGD(TAG, "Stopped buzzer");
}

void BuzzerComponent::apply_value_(uint8_t value) {
  ESP_LOGD(TAG, "Value: %d", value);
  if (this->output_ != nullptr) {
    this->output_->set_level(value / 255.0f);  // map 0–255 to float
  }
}

}  // namespace buzzer
}  // namespace esphome
