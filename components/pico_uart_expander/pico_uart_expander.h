#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/output/float_output.h"

namespace esphome {
namespace pico_uart_expander {

class PicoUARTExpanderComponent : public uart::UARTDevice, public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  void set_channel_value(uint8_t channel, uint8_t value);

 protected:
  void send_frame_();

  uint8_t data_[15] = {0};
  uint8_t last_data_[15] = {0};
  bool dirty_ = false;
};

class PicoUARTExpanderOutput : public output::FloatOutput {
 public:
  void set_parent(PicoUARTExpanderComponent *p) { parent_ = p; }
  void set_channel(uint8_t c) { channel_ = c; }

 protected:
  void write_state(float state) override {
    if (!parent_) return;
    if (state < 0.0f) state = 0.0f;
    if (state > 1.0f) state = 1.0f;
    const uint8_t val = static_cast<uint8_t>(state * 255.0f + 0.5f);
    parent_->set_channel_value(channel_, val);
  }

  PicoUARTExpanderComponent *parent_{nullptr};
  uint8_t channel_{0};
};

}  // namespace pico_uart_expander
}  // namespace esphome
