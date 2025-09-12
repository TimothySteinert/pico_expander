#pragma once

#include "esphome/core/component.h"

#ifdef USE_ESP32
#include "driver/uart.h"
#endif

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace esphome {
namespace buzzer { class BuzzerComponent; }
namespace script { class Script; class ScriptExecutor; }

namespace k1_uart {

class K1UartComponent : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;

  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  void set_buzzer(esphome::buzzer::BuzzerComponent *b) { buzzer_ = b; }

  void set_away_script(esphome::script::Script *s) { away_script_ = s; }
  void set_home_script(esphome::script::Script *s) { home_script_ = s; }
  void set_disarm_script(esphome::script::Script *s) { disarm_script_ = s; }

  void set_force_prefix(const std::string &v) { force_prefix_ = v; }
  void set_skip_delay_prefix(const std::string &v) { skip_delay_prefix_ = v; }

 protected:
#ifdef USE_ESP32
  // UART constants
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

  // Frame IDs & lengths
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

  // Parsing
  void parse_frames_();
  void handle_a0_(const uint8_t *frame, size_t len);
  void log_frame_(const uint8_t *data, size_t len, uint8_t id);

  // Digit & arm select mapping (from your previous handler)
  std::string map_digit_(uint8_t code) const;
  const char *map_arm_select_(uint8_t code) const;

  // Script invocation
  void call_arm_script_(esphome::script::Script *script,
                        const std::string &pin,
                        bool force_flag,
                        bool skip_flag);
  void call_disarm_script_(esphome::script::Script *script,
                           const std::string &prefix,
                           const std::string &pin,
                           bool force_flag,
                           bool skip_flag);
#endif

  // Linked components/scripts
  esphome::buzzer::BuzzerComponent *buzzer_{nullptr};
  esphome::script::Script *away_script_{nullptr};
  esphome::script::Script *home_script_{nullptr};
  esphome::script::Script *disarm_script_{nullptr};

  // Prefix rules
  std::string force_prefix_{"999"};
  std::string skip_delay_prefix_{"998"};
};

}  // namespace k1_uart
}  // namespace esphome
