#include "pico_uart_expander.h"
#include "esphome/core/log.h"
#include "driver/uart.h"

namespace esphome {
namespace pico_uart_expander {

static const char *const TAG = "pico_uart_expander";

void PicoUartExpanderComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up PicoUartExpander UART device...");
  // Initialize all data bytes to 0
  memset(data_bytes_, 0, sizeof(data_bytes_));
  
  // Get the UART port number from the parent component
  auto *idf_uart = static_cast<uart::IDFUARTComponent*>(this->parent_);
  uart_num_ = static_cast<uart_port_t>(idf_uart->get_hw_serial_number());
  
  ESP_LOGD(TAG, "Using UART port %d", uart_num_);
}

void PicoUartExpanderComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "PicoUartExpander (UART LED driver)");
  ESP_LOGCONFIG(TAG, "  UART Port: %d", uart_num_);
  this->check_uart_settings(115200);
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
  
  // Send complete message immediately
  send_uart_message();
}

void PicoUartExpanderComponent::send_uart_message() {
  // Create complete message: ID (0xB0) + 16 data bytes = 17 bytes total
  uint8_t message[17];
  message[0] = 0xB0;  // ID byte
  
  // Copy all 16 data bytes
  memcpy(&message[1], data_bytes_, 16);
  
  // Use native ESP32 IDF UART function for guaranteed atomic transmission
  int bytes_written = uart_write_bytes(uart_num_, message, 17);
  
  // Wait for transmission to complete before returning
  esp_err_t err = uart_wait_tx_done(uart_num_, pdMS_TO_TICKS(100));
  
  if (bytes_written != 17) {
    ESP_LOGE(TAG, "Failed to write complete message: wrote %d of 17 bytes", bytes_written);
  } else if (err != ESP_OK) {
    ESP_LOGE(TAG, "UART transmission failed: %s", esp_err_to_name(err));
  } else {
    ESP_LOGD(TAG, "Complete UART message sent atomically: 17 bytes");
  }
  
  // Debug log the complete message
  ESP_LOGD(TAG, "TX: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X", 
           message[0], message[1], message[2], message[3], message[4], message[5], message[6], message[7], message[8], 
           message[9], message[10], message[11], message[12], message[13], message[14], message[15], message[16]);
}

}  // namespace pico_uart_expander
}  // namespace esphome
