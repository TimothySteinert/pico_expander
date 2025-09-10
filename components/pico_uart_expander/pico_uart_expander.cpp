#include "pico_uart_expander.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pico_uart_expander {

static const char *const TAG = "pico_uart_expander";

void PicoUartExpanderComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up PicoUartExpander UART device...");
  // Initialize all data bytes to 0
  memset(data_bytes_, 0, sizeof(data_bytes_));
}

void PicoUartExpanderComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "PicoUartExpander (UART LED driver)");
  this->check_uart_settings(115200);  // Updated to match your baud rate
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with PicoUartExpander failed!");
  }
}

void PicoUartExpanderComponent::write_value(uint8_t channel, uint8_t value) {
  if (channel < 1 || channel > 16) {
    ESP_LOGW(TAG, "Invalid channel %d, must be 1-16", channel);
    return;
  }
  
  // Convert channel number to array index (channel 1-16 -> index 0-15)
  uint8_t index = channel - 1;
  
  // Update the data array
  data_bytes_[index] = value;
  
  ESP_LOGV(TAG, "Channel %d updated to 0x%02X", channel, value);
  
  // Always send complete message when any channel changes
  send_uart_message();
}

void PicoUartExpanderComponent::send_uart_message() {
  // Create complete message: ID (0xB0) + 16 data bytes = 17 bytes total
  uint8_t message[17];
  message[0] = 0xB0;  // ID byte
  
  // Copy all 16 data bytes (15 LED + 1 buzzer)
  memcpy(&message[1], data_bytes_, 16);
  
  // Send the complete message as one atomic write
  this->write_array(message, 17);
  this->flush();
  
  ESP_LOGV(TAG, "Complete UART message sent: 17 bytes");
  
  // Debug log the full message
  ESP_LOGD(TAG, "Message: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X", 
           message[0], message[1], message[2], message[3], message[4], message[5], message[6], message[7], message[8], 
           message[9], message[10], message[11], message[12], message[13], message[14], message[15], message[16]);
}

}  // namespace pico_uart_expander
}  // namespace esphome
