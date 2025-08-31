#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace pico_expander {

class PicoExpanderComponent : public Component, public i2c::I2CDevice {
 public:
  PicoExpanderComponent() = default;

  void setup() override;
  void loop() override;
  void dump_config() override;

  bool digital_read(uint8_t pin);
  void digital_write(uint8_t pin, bool value);
  void pin_mode(uint8_t pin, gpio::Flags flags);

  float get_setup_priority() const override { return setup_priority::IO; }

 protected:
  uint8_t last_state_[2]{0x00, 0x00};
};

class PicoExpanderGPIOPin : public GPIOPin {
 public:
  void setup() override;
  void pin_mode(gpio::Flags flags) override;

  bool digital_read() override;
  void digital_write(bool value) override;
  std::string dump_summary() const override;

  void set_parent(PicoExpanderComponent *parent) { this->parent_ = parent; }
  void set_pin(uint8_t pin) { this->pin_ = pin; }
  void set_inverted(bool inverted) { this->inverted_ = inverted; }
  void set_flags(gpio::Flags flags) { this->flags_ = flags; }
  gpio::Flags get_flags() const override { return this->flags_; }

 protected:
  PicoExpanderComponent *parent_;
  uint8_t pin_;
  bool inverted_{false};
  bool setup_{false};
  gpio::Flags flags_;
};

}  // namespace pico_expander
}  // namespace esphome
