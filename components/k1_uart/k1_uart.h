#include "k1_uart.h"
#include "esphome/core/log.h"
#include "buzzer/buzzer.h"   // bring full definition (path matches repo layout)

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

  if (uart_param_config(UART_PORT, &cfg) != ESP_OK) {
    ESP_LOGE(TAG, "uart_param_config failed");
    this->mark_failed();
    return;
  }
  if (uart_set_pin(UART_PORT, PIN_TX, PIN_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK) {
    ESP_LOGE(TAG, "uart_set_pin failed");
    this->mark_failed();
    return;
  }
  if (uart_driver_install(UART_PORT, RX_BUF_SIZE, TX_BUF_SIZE, QUEUE_SIZE, nullptr, 0) != ESP_OK) {
    ESP_LOGE(TAG, "uart_driver_install failed");
    this->mark_failed();
    return;
  }

  uart_flush_input(UART_PORT);
  ESP_LOGI(TAG, "UART ready (framed parser A0(21) A1(2), hex only).");
#endif
}

void K1UartComponent::loop() {
#ifdef USE_ESP32
  uint8_t buf[READ_CHUNK];
  int len = uart_read_bytes(UART_PORT, buf, sizeof(buf), 0);  // non-blocking
  if (len > 0) {
    for (int i = 0; i < len; i++) {
      push_byte_(buf[i]);
    }
    parse_frames_();
  }
#endif
}

void K1UartComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "K1 UART:");
#ifdef USE_ESP32
  ESP_LOGCONFIG(TAG, "  Port: UART1  Pins: TX=%d RX=%d  Baud=%d", PIN_TX, PIN_RX, BAUD);
  ESP_LOGCONFIG(TAG, "  Frames: A0=21 bytes, A1=2 bytes");
  ESP_LOGCONFIG(TAG, "  Buzzer linked: %s", buzzer_ ? "YES" : "NO");
#else
  ESP_LOGCONFIG(TAG, "  (Unsupported platform build)");
#endif
}

#ifdef USE_ESP32
// Ring buffer helpers
void K1UartComponent::push_byte_(uint8_t b) {
  if (ring_full_) {
    ring_tail_ = (ring_tail_ + 1) % RING_CAP;
  }
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
  size_t idx = (ring_tail_ + offset) % RING_CAP;
  return ring_[idx];
}
void K1UartComponent::pop_(size_t n) {
  if (n >= size_()) {
    ring_tail_ = ring_head_;
    ring_full_ = false;
    return;
  }
  ring_tail_ = (ring_tail_ + n) % RING_CAP;
  ring_full_ = false;
}

// Frame parsing
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

    // Trigger key beep on any A1 frame
    if (id == ID_A1 && buzzer_ != nullptr) {
      buzzer_->key_beep();
    }
  }
}

// Logging (hex only)
void K1UartComponent::log_frame_(const uint8_t *data, size_t len, uint8_t id) {
  char hex_part[70];
  size_t hpos = 0;
  for (size_t i = 0; i < len; i++) {
    if (hpos + 4 >= sizeof(hex_part)) break;
    hpos += snprintf(&hex_part[hpos], sizeof(hex_part) - hpos, "%02X ", data[i]);
  }
  if (hpos > 0 && hex_part[hpos - 1] == ' ')
    hex_part[hpos - 1] = 0;
  else
    hex_part[hpos] = 0;

  if (id == ID_A0) {
    ESP_LOGD(TAG, "A0 (%u): %s", (unsigned)len, hex_part);
  } else if (id == ID_A1) {
    ESP_LOGD(TAG, "A1 (%u): %s", (unsigned)len, hex_part);
  } else {
    ESP_LOGD(TAG, "ID %02X (%u): %s", id, (unsigned)len, hex_part);
  }
}
#endif  // USE_ESP32

}  // namespace k1_uart
}  // namespace esphome
