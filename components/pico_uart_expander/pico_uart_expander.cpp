#include "pico_uart_expander.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pico_uart_expander {

static const char *const TAG = "pico_uart_expander";

void PicoUARTExpanderComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Pico UART Expander...");
}

void PicoUARTExpanderComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Pico UART Expander (15-byte LED buffer)");
}

void PicoUARTExpanderComponent::loop() {
  if (!dirty_)
    return;

  // Compare with last_data_
  bool changed = false;
  for (int i = 0; i < 15; i++) {
    if (data_[i] != last_data_[i]) {
      changed = true;
      break;
    }
  }

  if (changed) {
    send_frame_();
    memcpy(last_data_, data_, sizeof(data_));
  }

  dirty_ = false;
}

void PicoUARTExpanderComponent::set_channel_value(uint8_t channel, uint8_t value) {
  if (channel < 1 || channel > 15)
    return;
  data_[channel - 1] = value;
  dirty_ = true;
}

void PicoUARTExpanderComponent::send_frame_() {
  const uint8_t id = 0xA0;
  const uint8_t crc = 0x00;  // placeholder
  this->write_byte(id);
  this->write_array(data_, 15);
  this->write_byte(crc);
  ESP_LOGV(TAG, "Sent frame ID=0x%02X", id);
}

}  // namespace pico_uart_expander
}  // namespace esphome
