#pragma once

#include "esphome/core/component.h"

#ifdef USE_ESP32
#include "driver/uart.h"
#endif

namespace esphome {
namespace k1_uart {

class K1UartComponent : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;

  // Changed: use HARDWARE (or remove this method)
  float get_setup_priority() const override {
    return setup_priority::HARDWARE;
  }

 protected:
#ifdef USE_ESP32
  static constexpr uart_port_t UART_PORT = UART_NUM_1;
  static constexpr int RX_BUF_SIZE = 1024;
  static constexpr int TX_BUF_SIZE = 0;
  static constexpr int QUEUE_SIZE = 0;
  static constexpr int BAUD = 115200;
  static constexpr int PIN_TX = 33;
  static constexpr int PIN_RX = 32;
  static constexpr int READ_CHUNK = 128;

  void log_bytes_(const uint8_t *data, int len);
#endif
};

}  // namespace k1_uart
}  // namespace esphome
