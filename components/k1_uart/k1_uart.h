#pragma once

#include "esphome/core/component.h"

#ifdef USE_ESP32
#include "driver/uart.h"
#endif

#include <array>
#include <cstdint>

namespace esphome {
namespace buzzer { class BuzzerComponent; }
namespace k1_arm_handler { class K1ArmHandlerComponent; }  // forward declaration

namespace k1_uart {

class K1UartComponent : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;

  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  void set_buzzer(esphome::buzzer::BuzzerComponent *b) { buzzer_ = b; }
  void set_arm_handler(esphome::k1_arm_handler::K1ArmHandlerComponent *h) { arm_handler_ = h; }

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

  // Ring buffer
  static constexpr size_t RING_CAP = 256;
  std::array<uint8_t, RING_CAP> ring_{};
  size_t ring_head_{0};
  size_t ring_tail_{0};
  bool ring_full_{false};

  // IDs & lengths
  static constexpr uint8_t ID_A0 = 0xA0;
  static constexpr uint8_t ID_A1 = 0xA1;
  static constexpr size_t LEN_A0 = 21;
  static constexpr size_t LEN_A1 = 2;

  // Buffer helpers
  void push_byte_(uint8_t b);
  bool available_() const;
  size_t size_() const;
  uint8_t peek_(size_t offset = 0) const;
  void pop_(size_t n);

  // Parsing & logging
  void parse_frames_();
  void log_frame_(const uint8_t *data, size_t len, uint8_t id);
#endif

  esphome::buzzer::BuzzerComponent *buzzer_{nullptr};
  esphome::k1_arm_handler::K1ArmHandlerComponent *arm_handler_{nullptr};
};

}  // namespace k1_uart
}  // namespace esphome
