#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/output/float_output.h"

namespace esphome {
namespace pico_expander {

/** Hub: I²C device exposing N 8-bit registers */
class PicoExpanderComponent : public Component, public i2c::I2CDevice {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::IO; }

  void write_value(uint8_t channel, uint8_t value);
};

/** LED Output: maps 0.0–1.0 float → 0x00–0xFF, writes to registers 0x30–0x3E. */
class PicoExpanderLedOutput : public output::FloatOutput {
 public:
  void set_parent(PicoExpanderComponent *parent) { parent_ = parent; }
  void set_channel(uint8_t channel) { channel_ = channel; }

 protected:
  void write_state(float state) override {
    if (!parent_) return;
    if (state < 0.0f) state = 0.0f;
    if (state > 1.0f) state = 1.0f;
    const uint8_t byte_val = static_cast<uint8_t>(state * 255.0f + 0.5f);
    parent_->write_value(channel_, byte_val);
  }

  PicoExpanderComponent *parent_{nullptr};
  uint8_t channel_{0};
};

/** Buzzer Output: supports duty (0x40) + frequency (0x41/0x42). */
class PicoExpanderBuzzerOutput : public output::FloatOutput {
 public:
  void set_parent(PicoExpanderComponent *parent) { parent_ = parent; }
  void set_channel(uint8_t channel) { channel_ = channel; }  // should be 0x40

  // RTTTL will call this with note frequency in Hz
  void set_frequency(float freq) override {
    if (!parent_) return;
    if (freq < 0) freq = 0;
    if (freq > 20000) freq = 20000;  // clamp to sane range
    uint16_t f = static_cast<uint16_t>(freq);
    parent_->write_value(0x41, f & 0xFF);        // low byte
    parent_->write_value(0x42, (f >> 8) & 0xFF); // high byte
  }

 protected:
  void write_state(float state) override {
    if (!parent_) return;
    if (state < 0.0f) state = 0.0f;
    if (state > 1.0f) state = 1.0f;
    const uint8_t byte_val = static_cast<uint8_t>(state * 255.0f + 0.5f);
    parent_->write_value(channel_, byte_val);  // duty to 0x40
  }

  PicoExpanderComponent *parent_{nullptr};
  uint8_t channel_{0};
};

}  // namespace pico_expander
}  // namespace esphome
