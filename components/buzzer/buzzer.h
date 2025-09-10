#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/core/hal.h"  // GPIOPin

namespace esphome {
namespace buzzer {

class BuzzerComponent : public Component {
 public:
  void setup() override {
    if (this->pin_ != nullptr) {
      this->pin_->setup();
      this->pin_->digital_write(false);
    }
  }

  void loop() override;

  void set_pin(GPIOPin *pin) { this->pin_ = pin; }

  void start(uint8_t beeps, uint32_t short_pause, uint32_t long_pause,
             uint8_t tone, bool repeat, uint32_t beep_length = 200);
  void stop();

  // Immediate 50ms overlay beep
  void key_beep();

  bool is_running() const { return this->running_; }

 protected:
  void apply_value_(uint8_t value);   // Updates pattern output state
  void refresh_output_();             // Applies combined (pattern OR key beep) to GPIO

  GPIOPin *pin_{nullptr};

  // Pattern state
  bool running_{false};
  bool beep_on_{false};
  bool repeat_{false};
  uint8_t beeps_{1};
  uint8_t beeps_done_{0};
  uint8_t tone_{255};
  uint32_t short_pause_{100};
  uint32_t long_pause_{500};
  uint32_t beep_length_{200};
  uint32_t current_interval_{0};
  uint32_t last_tick_{0};

  // Pattern output (before overlay)
  bool pattern_output_high_{false};

  // Key beep overlay
  bool key_beep_active_{false};
  uint32_t key_beep_end_{0};
  static constexpr uint32_t KEY_BEEP_LEN_MS = 50;
};

// ---- Actions ----
template<typename... Ts> class StartAction : public Action<Ts...> {
 public:
  explicit StartAction(BuzzerComponent *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(uint8_t, beeps)
  TEMPLATABLE_VALUE(uint32_t, short_pause)
  TEMPLATABLE_VALUE(uint32_t, long_pause)
  TEMPLATABLE_VALUE(uint8_t, tone)
  TEMPLATABLE_VALUE(bool, repeat)
  TEMPLATABLE_VALUE(uint32_t, beep_length)
  void play(Ts... x) override {
    this->parent_->start(this->beeps_.value(x...),
                         this->short_pause_.value(x...),
                         this->long_pause_.value(x...),
                         this->tone_.value(x...),
                         this->repeat_.value(x...),
                         this->beep_length_.value(x...));
  }
 private:
  BuzzerComponent *parent_;
};

template<typename... Ts> class StopAction : public Action<Ts...> {
 public:
  explicit StopAction(BuzzerComponent *parent) : parent_(parent) {}
  void play(Ts... x) override { this->parent_->stop(); }
 private:
  BuzzerComponent *parent_;
};

template<typename... Ts> class KeyBeepAction : public Action<Ts...> {
 public:
  explicit KeyBeepAction(BuzzerComponent *parent) : parent_(parent) {}
  void play(Ts... x) override { this->parent_->key_beep(); }
 private:
  BuzzerComponent *parent_;
};

}  // namespace buzzer
}  // namespace esphome
