#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/output/float_output.h"

namespace esphome {
namespace pico_expander {

class PicoExpanderComponent : public Component, public i2c::I2CDevice {
 public:
  void setup() override;
  void dump_config() override;
  void write_value(uint8_t channel, uint8_t value);
  float get_setup_priority() const override { return setup_priority::IO; }
};

class PicoExpanderOutput : public output::FloatOutput {
 public:
  void set_parent(PicoExpanderComponent *parent) { parent_ = parent; }
  void set_channel(uint8_t channel) { channel_ = channel; }

 protected:
  void write_state(float state) override {
    uint8_t val = static_cast<uint8_t>(state * 255.0f);
    parent_->write_value(channel_, val);
  }

  PicoExpanderComponent *parent_;
  uint8_t channel_;
};

}  // namespace pico_expander
}  // namespace esphome
