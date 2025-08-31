#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/output/float_output.h"

namespace esphome {
namespace pico_expander {

/**
 * Hub for a tiny I²C “expander” that exposes 3 registers:
 *   Reg 0 -> R (0x00..0xFF)
 *   Reg 1 -> G (0x00..0xFF)
 *   Reg 2 -> B (0x00..0xFF)
 */
class PicoExpanderComponent : public Component, public i2c::I2CDevice {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::IO; }

  void write_value(uint8_t channel, uint8_t value);

 protected:
};

/** A single FloatOutput channel (R or G or B) */
class PicoExpanderOutput : public output::FloatOutput {
 public:
  void set_parent(PicoExpanderComponent *parent) { parent_ = parent; }
  void set_channel(uint8_t channel) { channel_ = channel; }

 protected:
  // state is already clamped & transformed by ESPHome (min/max/inversion/ZMZ)
  void write_state(float state) override {
    // Map 0.0–1.0 → 0–255, rounded
    if (parent_ == nullptr) return;
    if (channel_ > 2) return;
    float s = state;
    if (s < 0.0f) s = 0.0f;
    if (s > 1.0f) s = 1.0f;
    const uint8_t byte_val = static_cast<uint8_t>(s * 255.0f + 0.5f);
    parent_->write_value(channel_, byte_val);
  }

  PicoExpanderComponent *parent_{nullptr};
  uint8_t channel_{0};
};

}  // namespace pico_expander
}  // namespace esphome
