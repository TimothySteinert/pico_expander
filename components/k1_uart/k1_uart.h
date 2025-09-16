#pragma once

#include "esphome/core/component.h"

#ifdef USE_ESP32
#include "driver/uart.h"
#include "esphome/components/script/script.h"
#include "esphome/components/select/select.h"
#include "esphome/components/argb_strip/argb_strip.h"
#include "esp_timer.h"
#endif

#include <array>
#include <cstdint>
#include <string>

namespace esphome {
namespace buzzer { class BuzzerComponent; }

namespace k1_uart {

#ifdef USE_ESP32
using AlarmScript = script::Script<std::string, bool, bool>;
#endif

class K1UartComponent : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  void set_buzzer(esphome::buzzer::BuzzerComponent *b) { buzzer_ = b; }

#ifdef USE_ESP32
  void set_away_script(AlarmScript *s) { away_script_ = s; }
  void set_home_script(AlarmScript *s) { home_script_ = s; }
  void set_disarm_script(AlarmScript *s) { disarm_script_ = s; }
  void set_night_script(AlarmScript *s) { night_script_ = s; }
  void set_vacation_script(AlarmScript *s) { vacation_script_ = s; }
  void set_bypass_script(AlarmScript *s) { bypass_script_ = s; }

  void set_mode_selector(select::Select *sel) { mode_selector_ = sel; }
  void set_arm_strip(argb_strip::ARGBStripComponent *s) { arm_strip_ = s; }
#endif

  void set_force_prefix(const std::string &v) { force_prefix_ = v; }
  void set_skip_delay_prefix(const std::string &v) { skip_delay_prefix_ = v; }
  void set_pinmode_timeout_ms(uint32_t v) { pinmode_timeout_ms_ = v; }

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

  static constexpr uint8_t ID_A0 = 0xA0;
  static constexpr uint8_t ID_A1 = 0xA1;
  static constexpr uint8_t ID_A3 = 0xA3;
  static constexpr uint8_t ID_A4 = 0xA4;
  static constexpr size_t LEN_A0 = 21;
  static constexpr size_t LEN_A1 = 2;
  static constexpr size_t LEN_A3 = 2;
  static constexpr size_t LEN_A4 = 2;

  static constexpr size_t RING_CAP = 256;
  std::array<uint8_t, RING_CAP> ring_{};
  size_t ring_head_{0};
  size_t ring_tail_{0};
  bool ring_full_{false};

  void push_byte_(uint8_t b);
  bool available_() const;
  size_t size_() const;
  uint8_t peek_(size_t offset = 0) const;
  void pop_(size_t n);

  void parse_frames_();
  void handle_a0_(const uint8_t *frame, size_t len);
  void handle_a3_(const uint8_t *frame, size_t len);
  void handle_a4_(const uint8_t *frame, size_t len);
  void log_frame_(const uint8_t *data, size_t len, uint8_t id);

  std::string map_digit_(uint8_t code) const;
  const char *map_arm_select_(uint8_t code) const;
  std::string map_dynamic_selector_option_() const;

  void exec_script_(AlarmScript *script,
                    const std::string &pin,
                    bool force_flag,
                    bool skip_flag);
  void handle_dynamic_mode_scripts_(const std::string &pin,
                                    bool force_flag,
                                    bool skip_flag);

  void apply_arm_select_mode_(const std::string &mode_name);

  // Pinmode (temporary tone mute) management
  void enter_pinmode_();
  void update_pinmode_timeout_(uint64_t now_us);
  void check_pinmode_expiry_(uint64_t now_us);
#endif

  esphome::buzzer::BuzzerComponent *buzzer_{nullptr};

#ifdef USE_ESP32
  AlarmScript *away_script_{nullptr};
  AlarmScript *home_script_{nullptr};
  AlarmScript *disarm_script_{nullptr};
  AlarmScript *night_script_{nullptr};
  AlarmScript *vacation_script_{nullptr};
  AlarmScript *bypass_script_{nullptr};

  select::Select *mode_selector_{nullptr};
  argb_strip::ARGBStripComponent *arm_strip_{nullptr};

  // Pinmode state
  bool pinmode_active_{false};
  uint64_t pinmode_last_activity_us_{0};
  uint32_t pinmode_timeout_ms_{2000};
#endif

  std::string force_prefix_{"999"};
  std::string skip_delay_prefix_{"998"};
};

}  // namespace k1_uart
}  // namespace esphome
