#pragma once
#include "esphome/components/switch/switch.h"
#include "esphome/core/component.h"
#include "../buzzer.h"

namespace esphome {
namespace buzzer {

enum class BuzzerMuteSwitchType : uint8_t { TONE = 0, BEEP = 1 };

class BuzzerMuteSwitch : public switch_::Switch, public Component {
 public:
  BuzzerMuteSwitch(BuzzerComponent *parent) : parent_(parent) {}

  void set_type(uint8_t t) { this->type_ = static_cast<BuzzerMuteSwitchType>(t); }

  void setup() override {
    // Publish current mute state (ON means muted)
    bool st = (this->type_ == BuzzerMuteSwitchType::TONE) ? parent_->tone_muted()
                                                          : parent_->beep_muted();
    this->publish_state(st);
  }

  void write_state(bool state) override {
    if (this->type_ == BuzzerMuteSwitchType::TONE) {
      if (state) parent_->tone_mute();
      else parent_->tone_unmute();
    } else {
      if (state) parent_->beep_mute();
      else parent_->beep_unmute();
    }
    // Parent will call publish_state() again via update_mute_switch_states, but
    // we can optimistically publish now to keep UI snappy.
    this->publish_state(state);
  }

  void sync_from_parent() {
    bool st = (this->type_ == BuzzerMuteSwitchType::TONE) ? parent_->tone_muted()
                                                          : parent_->beep_muted();
    this->publish_state(st);
  }

 protected:
  BuzzerComponent *parent_;
  BuzzerMuteSwitchType type_{BuzzerMuteSwitchType::TONE};
};

}  // namespace buzzer
}  // namespace esphome
