#include "buzzer.h"
#include "esphome/core/log.h"
#include "esp_timer.h"

namespace esphome {
namespace buzzer {

static const char *const TAG = "buzzer";

static inline uint32_t now_ms() {
  return (uint32_t) (esp_timer_get_time() / 1000ULL);
}

void BuzzerComponent::loop() {
  uint32_t now = now_ms();

  // Key beep expiration
  if (this->key_beep_active_ && now >= this->key_beep_end_) {
    this->key_beep_active_ = false;
    this->refresh_output_();
    ESP_LOGV(TAG, "Key beep ended");
  }

  if (!running_) return;

  if (now - last_tick_ < current_interval_) return;
  last_tick_ = now;

  if (beep_on_) {
    // Transition to OFF
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
    // Transition to ON
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
  this->last_tick_ = now_ms();
  this->current_interval_ = 0;

  if (beeps == 0 && !repeat) {
    this->running_ = false;
    apply_value_(0);
    ESP_LOGD(TAG, "Start with 0 beeps & no repeat: nothing to play");
    return;
  }

  ESP_LOGD(TAG, "Started: beeps=%u short=%ums long=%ums tone=%u repeat=%d len=%ums",
           beeps, short_pause, long_pause, tone, repeat, beep_length);
}

void BuzzerComponent::stop() {
  this->running_ = false;
  this->beep_on_ = false;
  apply_value_(0);
  ESP_LOGD(TAG, "Stopped");
}

void BuzzerComponent::key_beep() {
  uint32_t now = now_ms();
  this->key_beep_active_ = true;
  this->key_beep_end_ = now + KEY_BEEP_LEN_MS;
  this->refresh_output_();
  ESP_LOGV(TAG, "Key beep triggered (50ms)");
}

void BuzzerComponent::tone_mute() {
  if (!this->tone_muted_) {
    this->tone_muted_ = true;
    this->refresh_output_();
    ESP_LOGD(TAG, "Tone muted");
  }
}

void BuzzerComponent::tone_unmute() {
  if (this->tone_muted_) {
    this->tone_muted_ = false;
    this->refresh_output_();
    ESP_LOGD(TAG, "Tone unmuted");
  }
}

void BuzzerComponent::beep_mute() {
  if (!this->beep_muted_) {
    this->beep_muted_ = true;
    this->refresh_output_();
    ESP_LOGD(TAG, "Key beeps muted");
  }
}

void BuzzerComponent::beep_unmute() {
  if (this->beep_muted_) {
    this->beep_muted_ = false;
    this->refresh_output_();
    ESP_LOGD(TAG, "Key beeps unmuted");
  }
}

void BuzzerComponent::apply_value_(uint8_t value) {
  this->pattern_output_high_ = (value != 0);
  this->refresh_output_();
  ESP_LOGV(TAG, "Pattern logical=%s", this->pattern_output_high_ ? "HIGH" : "LOW");
}

void BuzzerComponent::refresh_output_() {
  if (this->pin_ != nullptr) {
    bool final_level =
        (this->pattern_output_high_ && !this->tone_muted_) ||
        (this->key_beep_active_ && !this->beep_muted_);
    this->pin_->digital_write(final_level);
  }
}

}  // namespace buzzer
}  // namespace esphome
