#pragma once

#include "esphome/core/component.h"

#ifdef USE_ESP32
#include "driver/uart.h"
#endif

#include <array>
#include <cstdint>

namespace esphome {
namespace k1_uart {

/*
  K1UartComponent
  - Hard-coded UART1 (TX=GPIO33, RX=GPIO32) 115200 8N1
  - Parses simple framed messages by leading ID byte:
        0xA0 -> fixed 21-byte frame (including ID)  (log all 21 bytes on ONE line)
        0xA1 -> fixed 2-byte frame  (ID + 1 data byte)
    Unknown ID: log single byte and advance (so we don't get stuck)
*/

class K1UartComponent : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;

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

  // Ring buffer for incremental parsing
  static constexpr size_t RING_CAP = 256;
  std::array<uint8_t, RING_CAP> ring_{};
  size_t ring_head_{0}; // write
  size_t ring_tail_{0}; // read
  bool ring_full_{false};

  void push_byte_(uint8_t b);
  bool available_() const;
  size_t size_() const;
  uint8_t peek_(size_t offset = 0) const;
  void pop_(size_t n);
  void parse_frames_();
  void log_frame_(const uint8_t *data, size_t len, uint8_t id);

  static constexpr uint8_t ID_A0 = 0xA0;
  static constexpr uint8_t ID_A1 = 0xA1;
  static constexpr size_t LEN_A0 = 21;
  static constexpr size_t LEN_A1 = 2;
#endif
};

}  // namespace k1_uart
}  // namespace esphome
