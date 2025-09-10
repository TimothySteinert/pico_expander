#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/output/float_output.h"

namespace esphome {
namespace pico_uart_expander {

/** Hub: UART device managing 15 LED channels + 1 buzzer channel */
class PicoUartExpanderComponent : public Component, public uart::UARTDevice {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::IO; }

  void write_value(uint8_t channel, uint8_t value);

 private:
  void send_uart_message();
  
  uint8_t data_bytes_[16] = {0};  // 15 LED channels + 1 buzzer channel
  bool data_changed_ = false;
};

/** Output: maps 0.0–1.0 float → 0x00–0xFF, writes one byte to data array. */
class PicoUartExpanderOutput : public output::FloatOutput {
 public:
  void set_parent(PicoUartExpanderComponent *parent) { parent_ = parent; }
  void set_channel(uint8_t channel) { channel_ = channel; }

 protected:
  // Called after ESPHome already applied min_power/max_power/inverted/zero_means_zero
  void write_state(float state) override {
    if (!parent_) return;
    if (state < 0.0f) state = 0.0f;
    if (state > 1.0f) state = 1.0f;
    const uint8_t byte_val = static_cast<uint8_t>(state * 255.0f + 0.5f);
    parent_->write_value(channel_, byte_val);
  }

  PicoUartExpanderComponent *parent_{nullptr};
  uint8_t channel_{0};
};

}  // namespace pico_uart_expander
}  // namespace esphome
