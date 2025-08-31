// pico_expander.h
#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace pico_expander {

class PicoExpanderComponent : public Component, public i2c::I2CDevice {
 public:
  PicoExpanderComponent() = default;

  void setup() override;
  void dump_config() override;

  void write_rgb(uint8_t r, uint8_t g, uint8_t b);

  float get_setup_priority() const override { return setup_priority::IO; }
};

}  // namespace pico_expander
}  // namespace esphome
