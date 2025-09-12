#include "k1_uart.h"
#include "esphome/core/log.h"

#ifdef USE_ESP32
#include "esphome/components/buzzer/buzzer.h"
#include "esphome/components/script/script.h"
#endif

namespace esphome {
namespace k1_uart {

static const char *const TAG = "k1_uart";

void K1UartComponent::setup() {
#ifndef USE_ESP32
  ESP_LOGE(TAG, "This component currently only supports ESP32 targets.");
  this->mark_failed();
  return;
#else
  ESP_LOGCONFIG(TAG, "Setting up K1 UART (UART1 @ %d baud, TX=%d RX=%d)", BAUD, PIN_TX, PIN_RX);

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
  ESP_LOGI(TAG, "UART ready (A0=21, A1=2, hex only).");
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
  ESP_LOGCONFIG(TAG, "  Port: UART1  Pins: TX=%d RX=%d  Baud=%d", PIN_TX, PIN_RX, BAUD);
  ESP_LOGCONFIG(TAG, "  Scripts: away=%s home=%s disarm=%s",
                away_script_ ? "YES":"NO",
                home_script_ ? "YES":"NO",
                disarm_script_ ? "YES":"NO");
  ESP_LOGCONFIG(TAG, "  force_prefix='%s' skip_delay_prefix='%s'",
                force_prefix_.c_str(), skip_delay_prefix_.c_str());
  ESP_LOGCONFIG(TAG, "  Buzzer linked: %s", buzzer_ ? "YES":"NO");
#endif
}

#ifdef USE_ESP32
// -------- Ring Buffer --------
void K1UartComponent::push_byte_(uint8_t b) {
  if (ring_full_) ring_tail_ = (ring_tail_ + 1) % RING_CAP;
  ring_[ring_head_] = b;
  ring_head_ = (ring_head_ + 1) % RING_CAP;
  ring_full_ = (ring_head_ == ring_tail_);
}
bool K1UartComponent::available_() const {
  return ring_full_ || (ring_head_ != ring_tail_);
}
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

// -------- Mapping Helpers (copied from prior handler logic) --------
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
    case 0x44: return "disarm";  // treat 0x44 same as 0x43 for now
    default:   return "unknown";
  }
}

// -------- Frame Parsing --------
void K1UartComponent::parse_frames_() {
  while (available_()) {
    uint8_t id = peek_();
    size_t needed = 0;
    if (id == ID_A0) needed = LEN_A0;
    else if (id == ID_A1) needed = LEN_A1;
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
    }
  }
}

// -------- A0 Handler -> Script Dispatch --------
void K1UartComponent::handle_a0_(const uint8_t *frame, size_t len) {
  if (len < LEN_A0 || frame[0] != ID_A0) {
    ESP_LOGW(TAG, "Invalid A0 frame length=%u first=0x%02X", (unsigned)len, frame[0]);
    return;
  }

  // Prefix bytes 1..3 (skip 0xFF)
  std::string prefix;
  for (int i = 1; i <= 3; i++) {
    if (frame[i] != 0xFF) {
      std::string d = map_digit_(frame[i]);
      if (d.empty()) {
        ESP_LOGV(TAG, "Ignoring unmapped prefix code 0x%02X", frame[i]);
      } else {
        prefix += d;
      }
    }
  }

  // Arm select byte 4
  const char *mode = "unknown";
  if (frame[4] != 0xFF)
    mode = map_arm_select_(frame[4]);

  // PIN bytes 5..20
  std::string pin;
  for (int i = 5; i < (int)LEN_A0; i++) {
    if (frame[i] != 0xFF) {
      std::string d = map_digit_(frame[i]);
      if (d.empty()) {
        ESP_LOGV(TAG, "Ignoring unmapped pin code 0x%02X", frame[i]);
      } else {
        pin += d;
      }
    }
  }

  if (std::string(mode) == "unknown" || pin.empty()) {
    ESP_LOGW(TAG, "A0 parse invalid mode=%s pin_len=%u", mode, (unsigned)pin.size());
    return;
  }

  bool force_flag = (!force_prefix_.empty() && prefix == force_prefix_);
  bool skip_flag  = (!skip_delay_prefix_.empty() && prefix == skip_delay_prefix_);

  ESP_LOGD(TAG, "A0 parsed: prefix='%s' mode=%s pin='%s' force=%s skip_delay=%s",
           prefix.c_str(), mode, pin.c_str(),
           force_flag ? "true":"false",
           skip_flag ? "true":"false");

  if (std::string(mode) == "disarm") {
    if (disarm_script_) {
      call_disarm_script_(disarm_script_, prefix, pin, force_flag, skip_flag);
    } else {
      ESP_LOGW(TAG, "Disarm script not configured");
    }
    return;
  }

  if (std::string(mode) == "away") {
    if (away_script_) {
      call_arm_script_(away_script_, pin, force_flag, skip_flag);
    } else {
      ESP_LOGW(TAG, "Away script not configured");
    }
  } else if (std::string(mode) == "home") {
    if (home_script_) {
      call_arm_script_(home_script_, pin, force_flag, skip_flag);
    } else {
      ESP_LOGW(TAG, "Home script not configured");
    }
  } else {
    ESP_LOGW(TAG, "Unhandled mode %s", mode);
  }
}

// -------- Script Calls --------
void K1UartComponent::call_arm_script_(esphome::script::Script *script,
                                       const std::string &pin,
                                       bool force_flag,
                                       bool skip_flag) {
  if (!script) return;
  script->execute([&](script::ScriptExecutor &se) {
    se.set_variable("pin", pin);
    se.set_variable("force", force_flag);
    se.set_variable("skip_delay", skip_flag);
  });
  ESP_LOGI(TAG, "Arm script executed (pin len=%u force=%d skip=%d)",
           (unsigned)pin.size(), (int)force_flag, (int)skip_flag);
}

void K1UartComponent::call_disarm_script_(esphome::script::Script *script,
                                          const std::string &prefix,
                                          const std::string &pin,
                                          bool force_flag,
                                          bool skip_flag) {
  if (!script) return;
  script->execute([&](script::ScriptExecutor &se) {
    se.set_variable("prefix", prefix);
    se.set_variable("pin", pin);
    se.set_variable("force", force_flag);
    se.set_variable("skip_delay", skip_flag);
  });
  ESP_LOGI(TAG, "Disarm script executed (pin len=%u force=%d skip=%d)",
           (unsigned)pin.size(), (int)force_flag, (int)skip_flag);
}

// -------- Logging --------
void K1UartComponent::log_frame_(const uint8_t *data, size_t len, uint8_t id) {
  char hex_part[80];
  size_t hpos = 0;
  for (size_t i = 0; i < len; i++) {
    if (hpos + 4 >= sizeof(hex_part)) break;
    hpos += snprintf(&hex_part[hpos], sizeof(hex_part) - hpos, "%02X ", data[i]);
  }
  if (hpos > 0 && hex_part[hpos - 1] == ' ')
    hex_part[hpos - 1] = 0;
  else
    hex_part[hpos] = 0;

  if (id == ID_A0)
    ESP_LOGD(TAG, "A0 (%u): %s", (unsigned)len, hex_part);
  else if (id == ID_A1)
    ESP_LOGD(TAG, "A1 (%u): %s", (unsigned)len, hex_part);
  else
    ESP_LOGD(TAG, "ID %02X (%u): %s", id, (unsigned)len, hex_part);
}
#endif  // USE_ESP32

}  // namespace k1_uart
}  // namespace esphome
