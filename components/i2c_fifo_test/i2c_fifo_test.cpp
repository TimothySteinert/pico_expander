#include "i2c_fifo_test.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace i2c_fifo_test {

static const char *const TAG = "i2c_fifo_test";

void I2CFifoTestComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up I2C FIFO Test...");
  this->data_acknowledged_ = true;
}

void I2CFifoTestComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "I2C FIFO Test:");
  LOG_I2C_DEVICE(this);
}

void I2CFifoTestComponent::loop() {
  this->poll_slave_status_();
}

void I2CFifoTestComponent::poll_slave_status_() {
  uint8_t status_bytes[2] = {0};
  
  // Read REG_READY (0x00) and REG_CTRL (0x01) in one transaction
  if (this->read_register(REG_READY, status_bytes, 2) != i2c::ERROR_OK) {
    ESP_LOGW(TAG, "Failed to read status registers");
    return;
  }
  
  uint8_t ready_byte = status_bytes[0];
  uint8_t ctrl_byte = status_bytes[1];
  
  ESP_LOGVV(TAG, "Status: READY=0x%02X, CTRL=0x%02X", ready_byte, ctrl_byte);
  
  // Check if new data is ready and we haven't already acknowledged it
  if (ready_byte == 0x01 && ctrl_byte == 0x00 && this->data_acknowledged_) {
    ESP_LOGI(TAG, "New data available, reading FIFO...");
    this->data_acknowledged_ = false;  // Mark that we're processing this data
    this->read_fifo_data_();
  }
  
  // If READY went back to 0x00, we can process new data again
  if (ready_byte == 0x00) {
    this->data_acknowledged_ = true;
  }
}

void I2CFifoTestComponent::read_fifo_data_() {
  uint8_t fifo_data[FIFO_SIZE] = {0};
  
  // Read all 32 FIFO bytes starting from FIFO_START (0x02)
  if (this->read_register(FIFO_START, fifo_data, FIFO_SIZE) != i2c::ERROR_OK) {
    ESP_LOGW(TAG, "Failed to read FIFO data");
    return;
  }
  
  // Log the received data in a readable format
  ESP_LOGI(TAG, "FIFO Data received:");
  
  // Log as hex bytes (16 bytes per line)
  for (int i = 0; i < FIFO_SIZE; i += 16) {
    String line = "  ";
    for (int j = 0; j < 16 && (i + j) < FIFO_SIZE; j++) {
      char hex_str[4];
      snprintf(hex_str, sizeof(hex_str), "%02X ", fifo_data[i + j]);
      line += hex_str;
    }
    ESP_LOGI(TAG, "%s", line.c_str());
  }
  
  // Also log specific keypad message if it matches expected pattern
  if (fifo_data[0] == 0xA0 && fifo_data[4] == 0x41) {
    ESP_LOGI(TAG, "Keypad sequence detected: A0 [%02X %02X %02X] 41 [%02X %02X %02X %02X]", 
             fifo_data[1], fifo_data[2], fifo_data[3], 
             fifo_data[5], fifo_data[6], fifo_data[7], fifo_data[8]);
  }
  
  // Send acknowledgment to clear the mailbox
  this->send_acknowledgment_();
}

void I2CFifoTestComponent::send_acknowledgment_() {
  uint8_t ack_value = CTRL_ACK;
  
  // Write 0xA5 to REG_CTRL (0x01) to acknowledge data received
  if (this->write_register(REG_CTRL, &ack_value, 1) != i2c::ERROR_OK) {
    ESP_LOGW(TAG, "Failed to send acknowledgment");
    return;
  }
  
  ESP_LOGI(TAG, "Acknowledgment sent (0xA5 -> REG_CTRL)");
}

}  // namespace i2c_fifo_test
}  // namespace esphome
