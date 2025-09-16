#include "k1_uart.h"
#include "esphome/core/log.h"

#ifdef USE_ESP32
#include "esphome/components/buzzer/buzzer.h"
#endif

namespace esphome {
namespace k1_uart {

static const char *const TAG = "k1_uart";

void K1UartComponent::setup() {
#ifndef USE_ESP32
  ESP_LOGE(TAG, "ESP32 only.");
  this->mark_failed();
  return;
#else
  uart_config_t cfg{};
  cfg.baud_rate = BAUD;
  cfg.data_bits = UART_DATA_8_BITS;
  cfg.parity = UART_PARITY_DISABLE;
  cfg.stop_bits = UART_STOP_BITS_1;
  cfg.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
  cfg.source_clk = UART_SCLK_DEFAULT;

  if (uart_param_config(UART_PORT, &cfg) != ESP_OK ||
      uart_set_pin(UART_PORT, PIN_TX, PIN_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK ||
      uart_driver_install(UART_PORT, RX_BUF_SIZE, TX_BUF_SIZE, QUEUE_SIZE, nullptr, 0) != ESP_OK) {
    ESP_LOGE(TAG, "UART init failed");
    this->mark_failed();
    return;
  }
  uart_flush_input(UART_PORT);
  ESP_LOGI(TAG, "UART ready (A0=21, A1=2, A3=2, A4=2).");
#endif
}

void K1UartComponent::loop() {
#ifdef USE_ESP32
  uint8_t buf[READ_CHUNK];
  int len = uart_read_bytes(UART_PORT, buf, sizeof(buf), 0);
  if (len > 0) {
    for (int i = 0; i < len; i++) push_byte_(buf[i]);
    parse_frames_();
  }
#endif
}

void K1UartComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "K1 UART:");
#ifdef USE_ESP32
  ESP_LOGCONFIG(TAG, "  Port: UART1 TX=%d RX=%d Baud=%d", PIN_TX, PIN_RX, BAUD);
  ESP_LOGCONFIG(TAG, "  Scripts (pin,force,skip): away=%s home=%s disarm=%s night=%s vacation=%s bypass=%s",
                away_script_ ? "YES":"NO",
                home_script_ ? "YES":"NO",
                disarm_script_ ? "YES":"NO",
                night_script_ ? "YES":"NO",
                vacation_script_ ? "YES":"NO",
                bypass_script_ ? "YES":"NO");
  ESP_LOGCONFIG(TAG, "  Dynamic selector: %s", mode_selector_ ? "YES":"NO");
  ESP_LOGCONFIG(TAG, "  Arm strip / RFID control: %s", arm_strip_ ? "YES":"NO");
  ESP_LOGCONFIG(TAG, "  force_prefix='%s' skip_delay_prefix='%s'",
                force_prefix_.c_str(), skip_delay_prefix_.c_str());
  ESP_LOGCONFIG(TAG, "  Buzzer: %s", buzzer_ ? "YES":"NO");
#endif
}

#ifdef USE_ESP32
// -------- Ring buffer --------
void K1UartComponent::push_byte_(uint8_t b) {
  if (ring_full_) ring_tail_ = (ring_tail_ + 1) % RING_CAP;
  ring_[ring_head_] = b;
  ring_head_ = (ring_head_ + 1) % RING_CAP;
  ring_full_ = (ring_head_ == ring_tail_);
}
bool K1UartComponent::available_() const { return ring_full_ || (ring_head_ != ring_tail_); }
size_t K1UartComponent::size_() const {
  if (ring_full_) return RING_CAP;
  if (ring_head_ >= ring_tail_) return ring_head_ - ring_tail_;
  return RING_CAP - (ring_tail_ - ring_head_);
}
uint8_t K1UartComponent::peek_(size_t offset) const {
  if (offset >= size_()) return 0;
  return ring_[(ring_tail_ + offset) % RING_CAP];
}
void K1UartComponent::pop_(size_t n) {
  if (n >= size_()) {
    ring_tail_ = ring_head_;
    ring_full_ = false;
  } else {
    ring_tail_ = (ring_tail_ + n) % RING_CAP;
    ring_full_ = false;
  }
}

// -------- Mapping (digits) --------
std::string K1UartComponent::map_digit_(uint8_t code) const {
  switch (code) {
    case 0x05: return "1";
    case 0x0A: return "2";
    case 0x0F: return "3";
    case 0x11: return "4";
    case 0x16: return "5";
    case 0x1B: return "6";
    case 0x1C: return "7";
    case 0x22: return "8";
    case 0x27: return "9";
    case 0x00: return "0";
    default:   return "";
  }
}
const char *K1UartComponent::map_arm_select_(uint8_t code) const {
  switch (code) {
    case 0x41: return "away";
    case 0x42: return "home";
    case 0x43: return "disarm";
    case 0x44: return "dynamic";
    default:   return "unknown";
  }
}

std::string K1UartComponent::map_dynamic_selector_option_() const {
  if (!mode_selector_) return {};
  std::string sel = mode_selector_->state;
  std::string lc;
  lc.resize(sel.size());
  std::transform(sel.begin(), sel.end(), lc.begin(),
                 [](unsigned char c){ return (char) std::tolower(c); });
  if (lc == "night") return "night";
  if (lc == "vacation") return "vacation";
  if (lc == "custom bypass" || lc == "bypass" || lc == "custom_bypass") return "bypass";
  return {};
}

// -------- Parser main --------
void K1UartComponent::parse_frames_() {
  while (available_()) {
    uint8_t id = peek_();
    size_t needed = 0;
    if (id == ID_A0) needed = LEN_A0;
    else if (id == ID_A1) needed = LEN_A1;
    else if (id == ID_A3) needed = LEN_A3;
    else if (id == ID_A4) needed = LEN_A4;   // NEW
    else {
      ESP_LOGD(TAG, "UNK: %02X", id);
      pop_(1);
      continue;
    }
    if (size_() < needed) break;

    uint8_t frame[LEN_A0];
    for (size_t i = 0; i < needed; i++) frame[i] = peek_(i);

    log_frame_(frame, needed, id);
    pop_(needed);

    if (id == ID_A1) {
      if (buzzer_) buzzer_->key_beep();
    } else if (id == ID_A0) {
      handle_a0_(frame, needed);
    } else if (id == ID_A3) {
      handle_a3_(frame, needed);
    } else if (id == ID_A4) {
      handle_a4_(frame, needed);
    }
  }
}

// -------- A0 (scripts) --------
void K1UartComponent::handle_a0_(const uint8_t *frame, size_t len) {
  if (len != LEN_A0 || frame[0] != ID_A0) return;

  // Prefix for flags
  std::string prefix;
  for (int i = 1; i <= 3; i++) {
    if (frame[i] != 0xFF) {
      auto d = map_digit_(frame[i]);
      if (!d.empty()) prefix += d;
    }
  }

  const char *mode = (frame[4] == 0xFF) ? "unknown" : map_arm_select_(frame[4]);

  std::string pin;
  for (int i = 5; i < (int)LEN_A0; i++) {
    if (frame[i] != 0xFF) {
      auto d = map_digit_(frame[i]);
      if (!d.empty()) pin += d;
    }
  }

  if (std::string(mode) == "unknown" || pin.empty()) {
    ESP_LOGW(TAG, "A0 invalid mode=%s pin_len=%u", mode, (unsigned)pin.size());
    return;
  }

  bool force_flag = (!force_prefix_.empty() && prefix == force_prefix_);
  bool skip_flag  = (!skip_delay_prefix_.empty() && prefix == skip_delay_prefix_);

  ESP_LOGD(TAG, "A0 parsed: mode=%s prefix='%s' pin='%s' force=%d skip=%d",
           mode, prefix.c_str(), pin.c_str(), (int)force_flag, (int)skip_flag);

  std::string m = mode;
  if (m == "disarm") {
    exec_script_(disarm_script_, pin, force_flag, skip_flag);
  } else if (m == "away") {
    exec_script_(away_script_, pin, force_flag, skip_flag);
  } else if (m == "home") {
    exec_script_(home_script_, pin, force_flag, skip_flag);
  } else if (m == "dynamic") {
    handle_dynamic_mode_scripts_(pin, force_flag, skip_flag);
  } else {
    ESP_LOGW(TAG, "Unhandled mode %s", mode);
  }
}

void K1UartComponent::handle_dynamic_mode_scripts_(const std::string &pin,
                                                   bool force_flag,
                                                   bool skip_flag) {
  auto dyn = map_dynamic_selector_option_();
  AlarmScript *target = nullptr;
  if (dyn == "night") target = night_script_;
  else if (dyn == "vacation") target = vacation_script_;
  else if (dyn == "bypass") target = bypass_script_;

  if (dyn.empty()) {
    ESP_LOGW(TAG, "Dynamic (A0 0x44) selector state invalid / missing");
    return;
  }
  if (!target) {
    ESP_LOGW(TAG, "No script configured for dynamic state '%s'", dyn.c_str());
    return;
  }
  exec_script_(target, pin, force_flag, skip_flag);
}

// -------- A3 (arm select LED control) --------
void K1UartComponent::handle_a3_(const uint8_t *frame, size_t len) {
  if (len != LEN_A3 || frame[0] != ID_A3) return;
  uint8_t code = frame[1];

  std::string final_mode;

  if (code == 0xFF) {
    final_mode = "none";
  } else if (code == 0x41) {
    final_mode = "away";
  } else if (code == 0x42) {
    final_mode = "home";
  } else if (code == 0x43) {
    final_mode = "disarm";
  } else if (code == 0x44) {
    auto dyn = map_dynamic_selector_option_();
    if (dyn.empty()) {
      ESP_LOGW(TAG, "A3 dynamic 0x44 but selector invalid -> none");
      final_mode = "none";
    } else {
      final_mode = dyn;
    }
  } else {
    ESP_LOGW(TAG, "A3 unknown code 0x%02X -> none", code);
    final_mode = "none";
  }

  ESP_LOGD(TAG, "A3 set arm-select mode -> %s", final_mode.c_str());
  apply_arm_select_mode_(final_mode);
}

// -------- A4 (RFID strip mode control) --------
void K1UartComponent::handle_a4_(const uint8_t *frame, size_t len) {
  if (len != LEN_A4 || frame[0] != ID_A4) return;
  if (!arm_strip_) {
    ESP_LOGV(TAG, "A4 received but no arm strip configured");
    return;
  }
  uint8_t code = frame[1];
  if (code == 0x20) {
    ESP_LOGD(TAG, "A4: Set strip NORMAL (disable RFID)");
    arm_strip_->disable_rfid_mode();
  } else if (code == 0x30) {
    ESP_LOGD(TAG, "A4: Set strip RFID (enable RFID)");
    arm_strip_->enable_rfid_mode();
  } else {
    ESP_LOGW(TAG, "A4 unknown code 0x%02X (expected 0x20/0x30)", code);
  }
}

// -------- Apply arm-select LED mode --------
void K1UartComponent::apply_arm_select_mode_(const std::string &mode_name) {
  if (!arm_strip_) {
    ESP_LOGV(TAG, "Arm strip not configured; ignoring arm-select mode set");
    return;
  }
  arm_strip_->set_arm_select_mode_by_name(mode_name.c_str());
}

// -------- Script exec --------
void K1UartComponent::exec_script_(AlarmScript *script,
                                   const std::string &pin,
                                   bool force_flag,
                                   bool skip_flag) {
  if (!script) {
    ESP_LOGW(TAG, "Script not configured for this mode");
    return;
  }
  script->execute(pin, force_flag, skip_flag);
  ESP_LOGI(TAG, "Script executed (pin_len=%u force=%d skip=%d)",
           (unsigned) pin.size(), (int)force_flag, (int)skip_flag);
}

// -------- Logging --------
void K1UartComponent::log_frame_(const uint8_t *data, size_t len, uint8_t id) {
  char hex_part[64];
  size_t hpos = 0;
  for (size_t i = 0; i < len; i++) {
    if (hpos + 4 >= sizeof(hex_part)) break;
    hpos += snprintf(&hex_part[hpos], sizeof(hex_part) - hpos, "%02X ", data[i]);
  }
  if (hpos > 0 && hex_part[hpos - 1] == ' ')
    hex_part[hpos - 1] = 0;
  else
    hex_part[hpos] = 0;

  if (id == ID_A0) ESP_LOGD(TAG, "A0 (%u): %s", (unsigned)len, hex_part);
  else if (id == ID_A1) ESP_LOGD(TAG, "A1 (%u): %s", (unsigned)len, hex_part);
  else if (id == ID_A3) ESP_LOGD(TAG, "A3 (%u): %s", (unsigned)len, hex_part);
  else if (id == ID_A4) ESP_LOGD(TAG, "A4 (%u): %s", (unsigned)len, hex_part);
  else ESP_LOGD(TAG, "ID %02X (%u): %s", id, (unsigned)len, hex_part);
}
#endif  // USE_ESP32

}  // namespace k1_uart
}  // namespace esphome
