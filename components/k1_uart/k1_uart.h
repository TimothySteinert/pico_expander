#pragma once

#include "esphome/core/component.h"

#ifdef USE_ESP32
#include "driver/uart.h"
#endif

namespace esphome {
namespace k1_uart {

/*
  K1UartComponent
  - Hard-coded UART on ESP32:
      Port: UART1
      TX: GPIO33
      RX: GPIO32
      Baud: 115200 8N1
  - Reads any incoming bytes and logs them in hex (& printable ASCII)
  - No user configuration exposed (schema only provides an id)
*/

class K1UartComponent : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;

  float get_setup_priority() const override {
    // After hardware is fine; standard priority OK.
    return setup_priority::HARDWARE_LATE;
  }

 protected:
#ifdef USE_ESP32
  static constexpr uart_port_t UART_PORT = UART_NUM_1;
  static constexpr int RX_BUF_SIZE = 1024;
  static constexpr int TX_BUF_SIZE = 0;     // We only receive for now
  static constexpr int QUEUE_SIZE = 0;      // No event queue yet
  static constexpr int BAUD = 115200;
  static constexpr int PIN_TX = 33;
  static constexpr int PIN_RX = 32;
  static constexpr int READ_CHUNK = 128;

  void log_bytes_(const uint8_t *data, int len);
#endif
};

}  // namespace k1_uart
}  // namespace esphome
