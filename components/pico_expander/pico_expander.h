#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/output/float_output.h"

namespace esphome {
namespace pico_expander {

/** Hub device exposing 3 8-bit I²C registers (0=R, 1=G, 2=B). */
class PicoExpanderComponent : public Component, public i2c::I2CDevice {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::IO; }

  void write_value(uint8_t channel, uint8_t value);
};

/** Single FloatOutput channel mapping 0.0–1.0 to 0x00–0xFF and writing one byte */
class PicoExpanderOutput : public output::FloatOutput {
 public:
  void set_parent(PicoExpanderComponent *parent) { parent_ = parent; }
  void set_channel(uint8_t channel) { channel_ = channel; }

 protected:
  // FloatOutput already applied min/max/inversion/zero_means_zero
  void write_state(float state) override {
    if (!parent_ || channel_ > 2) return;
    if (state < 0.0f) state = 0.0f;
    if (state > 1.0f) state = 1.0f;
    const uint8_t byte_val = static_cast<uint8_t>(state * 255.0f + 0.5f);
    parent_->write_value(channel_, byte_val);
  }

  PicoExpanderComponent *parent_{nullptr};
  uint8_t channel_{0};
};

}  // namespace pico_expander
}  // namespace esphome
