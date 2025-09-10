#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace i2c_fifo_test {

class I2CFifoTestComponent : public Component, public i2c::I2CDevice {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;

 protected:
  bool data_acknowledged_{true}; // Track if we've already read this data

  // I2C slave register addresses
  static constexpr uint8_t REG_READY = 0x00;
  static constexpr uint8_t REG_CTRL = 0x01;
  static constexpr uint8_t FIFO_START = 0x02;
  static constexpr uint8_t FIFO_SIZE = 32;
  static constexpr uint8_t CTRL_ACK = 0xA5;

  void poll_slave_status_();
  void read_fifo_data_();
  void send_acknowledgment_();
};

}  // namespace i2c_fifo_test
}  // namespace esphome
