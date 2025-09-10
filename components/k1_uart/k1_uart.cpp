#include "k1_uart.h"
#include "esphome/core/log.h"

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

  // Flush any junk that might be present
  uart_flush_input(UART_PORT);
  ESP_LOGI(TAG, "UART ready.");
#endif
}

void K1UartComponent::loop() {
#ifdef USE_ESP32
  uint8_t buf[READ_CHUNK];
  int len = uart_read_bytes(UART_PORT, buf, sizeof(buf), 0);  // non-blocking
  if (len > 0) {
    log_bytes_(buf, len);
  }
#endif
}

void K1UartComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "K1 UART:");
#ifdef USE_ESP32
  ESP_LOGCONFIG(TAG, "  Port: UART1");
  ESP_LOGCONFIG(TAG, "  Pins: TX=%d RX=%d", PIN_TX, PIN_RX);
  ESP_LOGCONFIG(TAG, "  Baud: %d", BAUD);
  ESP_LOGCONFIG(TAG, "  RX Buffer: %d", RX_BUF_SIZE);
#else
  ESP_LOGCONFIG(TAG, "  (Unsupported platform build)");
#endif
}

#ifdef USE_ESP32
// Helper: log bytes as hex + ASCII (similar to a hexdump line grouping)
void K1UartComponent::log_bytes_(const uint8_t *data, int len) {
  // You can adjust verbosity. Using DEBUG so it shows with default INFO+ if needed change to VERBOSE.
  const int per_line = 16;
  for (int i = 0; i < len; i += per_line) {
    int chunk = std::min(per_line, len - i);
    char hex_part[per_line * 3 + 1];
    char ascii_part[per_line + 1];
    int hpos = 0;
    int apos = 0;
    for (int j = 0; j < chunk; j++) {
      uint8_t b = data[i + j];
      hpos += snprintf(&hex_part[hpos], sizeof(hex_part) - hpos, "%02X ", b);
      ascii_part[apos++] = (b >= 32 && b < 127) ? (char)b : '.';
    }
    hex_part[hpos] = 0;
    ascii_part[apos] = 0;
    ESP_LOGD(TAG, "%04X: %-48s | %s", i, hex_part, ascii_part);
  }
}
#endif

}  // namespace k1_uart
}  // namespace esphome
