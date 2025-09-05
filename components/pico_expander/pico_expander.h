#pragma once

#include "esphome/core/component.h"
#include "esphome/core/gpio.h"              // 🔹 For GPIOPin
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

/** LED Output: maps 0.0–1.0 float → 0x00–0xFF, writes one byte to register. */
class PicoExpanderOutput : public output::FloatOutput {
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

/** GPIO Pin: writes 0x00 (OFF) or 0x11 (ON) to configurable register (default 0x40). */
/** GPIO Pin: writes 0x00 (OFF) or 0x11 (ON) to configurable register (0x40–0x4F). */
class PicoExpanderGPIOPin : public GPIOPin {
 public:
  void set_parent(PicoExpanderComponent *parent) { parent_ = parent; }
  void set_channel(uint8_t channel) { channel_ = channel; }
  void set_flags(gpio::Flags flags) { this->flags_ = flags; }
  void set_inverted(bool inverted) { this->inverted_ = inverted; }

  void setup() override {}

  void pin_mode(gpio::Flags flags) override {
    // For now, we only support output, but keep the value for debugging
    this->flags_ = flags;
  }

  void digital_write(bool value) override {
    if (!parent_) return;
    bool out = inverted_ ? !value : value;
    const uint8_t byte_val = out ? 0x11 : 0x00;
    parent_->write_value(channel_, byte_val);
  }

  bool digital_read() override {
    // Inputs not supported, always return false
    return false;
  }

  std::string dump_summary() const override {
    char buffer[64];
    snprintf(buffer, sizeof(buffer),
             "PicoExpander GPIO reg=0x%02X (flags=%u, inverted=%d)",
             channel_, static_cast<unsigned>(flags_), inverted_);
    return buffer;
  }

 protected:
  PicoExpanderComponent *parent_{nullptr};
  uint8_t channel_{0};   // Will always be set by init.py
  gpio::Flags flags_{};
  bool inverted_{false};
};


}  // namespace pico_expander
}  // namespace esphome
