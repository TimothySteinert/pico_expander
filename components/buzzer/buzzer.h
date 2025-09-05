#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/components/output/float_output.h"

namespace esphome {
namespace buzzer {

class BuzzerComponent : public Component {
 public:
  void setup() override {}
  void loop() override;

  void set_output(output::FloatOutput *output) { this->output_ = output; }

  void start(uint8_t beeps, uint32_t short_pause, uint32_t long_pause,
             uint8_t tone, bool repeat, uint32_t beep_length = 200);
  void stop();

 protected:
  void apply_value_(uint8_t value);

  output::FloatOutput *output_{nullptr};

  bool running_{false};
  bool beep_on_{false};
  bool repeat_{false};
  uint8_t beeps_{1};
  uint8_t beeps_done_{0};
  uint8_t tone_{128};
  uint32_t short_pause_{100};
  uint32_t long_pause_{500};
  uint32_t beep_length_{200};
  uint32_t current_interval_{0};
  uint32_t last_tick_{0};
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
 protected:
  BuzzerComponent *parent_;
};

template<typename... Ts> class StopAction : public Action<Ts...> {
 public:
  explicit StopAction(BuzzerComponent *parent) : parent_(parent) {}
  void play(Ts... x) override { this->parent_->stop(); }
 protected:
  BuzzerComponent *parent_;
};

}  // namespace buzzer
}  // namespace esphome
