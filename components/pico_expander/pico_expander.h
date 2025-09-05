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
class PicoExpanderGPIOPin : public GPIOPin {
 public:
  void set_parent(PicoExpanderComponent *parent) { parent_ = parent; }
  void set_channel(uint8_t channel) { channel_ = channel; }

  void setup() override {}  // nothing to init
  void pin_mode(gpio::Flags flags) override { (void) flags; }  // only output supported

  void digital_write(bool value) override {
    if (!parent_) return;
    const uint8_t byte_val = value ? 0x11 : 0x00;
    parent_->write_value(channel_, byte_val);
  }

  bool digital_read() override { return false; }  // not implemented

  std::string dump_summary() const override {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "PicoExpander GPIO reg=0x%02X", channel_);
    return buffer;
  }

 protected:
  PicoExpanderComponent *parent_{nullptr};
  uint8_t channel_{0x40};
};

}  // namespace pico_expander
}  // namespace esphome
