#include "pico_panel_base.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"      // provides esphome::millis()
#include "esphome/core/helpers.h"  // provides esphome::delay()

namespace esphome {
namespace pico_panel_base {

static const char *const TAG = "pico_panel_base";

// CRC-8 poly 0x07
uint8_t PicoPanelBase::crc8(const uint8_t *data, size_t len) {
  uint8_t crc = 0x00;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t b = 0; b < 8; b++)
      crc = (crc & 0x80) ? uint8_t((crc << 1) ^ 0x07) : uint8_t(crc << 1);
  }
  return crc;
}

// I2C helpers using ESPHome I2CDevice API (handles pointer write + repeated START)
bool PicoPanelBase::read_u8(uint8_t reg, uint8_t &val) {
  return this->read_register(reg, &val, 1) == i2c::ERROR_OK;
}
bool PicoPanelBase::write_u8(uint8_t reg, uint8_t val) {
  return this->write_register(reg, &val, 1) == i2c::ERROR_OK;
}

bool PicoPanelBase::wait_status(uint8_t bit_mask, uint32_t timeout_ms) {
  const uint32_t start = esphome::millis();
  while (esphome::millis() - start < timeout_ms) {
    uint8_t st = 0;
    if (read_u8(REG_STATUS0, st)) {
      if (st & ST_ERROR) {
        uint8_t e = 0;
        read_u8(REG_ERROR, e);
        ESP_LOGE(TAG, "STATUS error (err=%u)", e);
        return false;
      }
      if (st & bit_mask) return true;
    }
    esphome::delay(50);
  }
  return false;
}

void PicoPanelBase::setup() {
  ESP_LOGI(TAG, "Setting up PicoPanelBase at 0x%02X ...", this->address_);

  // 1) WHOAMI
  uint8_t id = 0;
  if (!read_u8(REG_DEVICE_ID, id)) {
    ESP_LOGE(TAG, "Failed to read DEVICE_ID");
    this->mark_failed();
    return;
  }
  if (id != 0xA5) {
    ESP_LOGE(TAG, "Unexpected DEVICE_ID 0x%02X (want 0xA5)", id);
    this->mark_failed();
    return;
  }
  ESP_LOGD(TAG, "WHOAMI OK (0xA5)");

  // 2) Soft reset → wait READY (minimal sequencing)
  (void) write_u8(REG_CONTROL0, CTL_SOFT_RESET);
  if (!wait_status(ST_READY, 1000)) {
    ESP_LOGE(TAG, "READY timeout after reset");
    this->mark_failed();
    return;
  }
  online_ = true;
  last_ms_ = esphome::millis();
  ESP_LOGI(TAG, "✅ Base comms online");
}

void PicoPanelBase::loop() {
  if (!online_) return;

  // once per second: send KEEPALIVE, read a couple of counters
  const uint32_t now = esphome::millis();
  if (now - last_ms_ >= 1000) {
    (void) write_u8(REG_CONTROL0, CTL_KEEPALIVE);

    uint8_t alive = 0, ver = 0, st = 0;
    read_u8(REG_ALIVE, alive);
    read_u8(REG_VERSION, ver);
    read_u8(REG_STATUS0, st);

    ESP_LOGV(TAG, "Alive=%u Version=%u Status=0x%02X", alive, ver, st);
    last_ms_ = now;
  }
}

void PicoPanelBase::dump_config() {
  ESP_LOGCONFIG(TAG, "PicoPanelBase:");
  LOG_I2C_DEVICE(this);
  ESP_LOGCONFIG(TAG, "  Minimal mode: WHOAMI + keepalive; no config staging.");
}

}  // namespace pico_panel_base
}  // namespace esphome
