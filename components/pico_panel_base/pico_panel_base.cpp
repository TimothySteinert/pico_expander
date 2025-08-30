#include "pico_panel_base.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pico_panel_base {

static const char *const TAG = "pico_panel_base";

// ---- CRC-8 poly 0x07 ----
uint8_t PicoPanelBase::crc8(const uint8_t *data, size_t len) {
  uint8_t crc = 0x00;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t b = 0; b < 8; b++)
      crc = (crc & 0x80) ? uint8_t((crc << 1) ^ 0x07) : uint8_t(crc << 1);
  }
  return crc;
}

// ---- I2C tiny helpers (use ESPHome I2CDevice API) ----
// NOTE: read_register/write_register perform the pointer write + repeated START read for us. :contentReference[oaicite:5]{index=5}
bool PicoPanelBase::read_u8(uint8_t reg, uint8_t &val) {
  return this->read_register(reg, &val, 1) == i2c::ERROR_OK;
}
bool PicoPanelBase::write_u8(uint8_t reg, uint8_t val) {
  return this->write_register(reg, &val, 1) == i2c::ERROR_OK;
}
bool PicoPanelBase::read_block(uint8_t reg, uint8_t *buf, size_t len) {
  return this->read_register(reg, buf, len) == i2c::ERROR_OK;
}
bool PicoPanelBase::write_block(uint8_t reg, const uint8_t *buf, size_t len) {
  return this->write_register(reg, buf, len) == i2c::ERROR_OK;
}

bool PicoPanelBase::wait_status(uint8_t bit_mask, uint32_t timeout_ms) {
  const uint32_t start = millis();
  while (millis() - start < timeout_ms) {
    uint8_t st = 0;
    if (read_u8(REG_STATUS0, st) && (st & ST_ERROR)) {
      uint8_t e = 0;
      read_u8(REG_ERROR, e);
      ESP_LOGE(TAG, "STATUS error flag set (err=%u)", e);
      return false;
    }
    if (st & bit_mask) return true;
    delay(50);
  }
  return false;
}

bool PicoPanelBase::stage_apply_cfg(uint8_t cfg_sel, const uint8_t *data, uint8_t len) {
  if (!write_u8(REG_CFG_SEL, cfg_sel)) return false;
  if (!write_u8(REG_CFG_LEN, len)) return false;
  if (!write_block(REG_CFG_BASE, data, len)) return false;

  // START_CONFIG
  if (!write_u8(REG_CONTROL0, CTL_START_CONFIG)) return false;
  if (!wait_status(ST_CFG_STAGED, 1000)) {
    ESP_LOGW(TAG, "Config stage timeout");
    return false;
  }

  // APPLY_CONFIG
  if (!write_u8(REG_CONTROL0, CTL_APPLY_CONFIG)) return false;
  if (!wait_status(ST_CONFIGURED, 1000)) {
    ESP_LOGW(TAG, "Config apply timeout");
    return false;
  }
  return true;
}

void PicoPanelBase::setup() {
  ESP_LOGI(TAG, "Setting up PicoPanelBase at 0x%02X ...", this->address_);
  // 1) WHOAMI
  uint8_t id = 0x00;
  if (!read_u8(REG_DEVICE_ID, id) || id != 0xA5) {
    ESP_LOGE(TAG, "Bad/missing ID (got 0x%02X)", id);
    this->mark_failed();
    return;
  }
  ESP_LOGD(TAG, "WHOAMI OK (0xA5)");

  // 2) SOFT_RESET → wait READY
  ESP_LOGD(TAG, "Issuing SOFT_RESET");
  write_u8(REG_CONTROL0, CTL_SOFT_RESET);
  if (!wait_status(ST_READY, 1000)) {
    ESP_LOGE(TAG, "READY timeout after reset");
    this->mark_failed();
    return;
  }
  ESP_LOGI(TAG, "Slave READY");

  // Prepare two tiny config windows (RFID & SYS), each ending with CRC
  uint8_t rfid[16] = {0};
  rfid[0] = 0x01;          // version
  rfid[1] = rfid_key_id_;  // key id
  rfid[2] = rfid_flags_;   // flags
  rfid[15] = crc8(rfid, 15);

  uint8_t sys[16] = {0};
  sys[0] = 0x01;           // version
  sys[1] = sys_options_;
  sys[15] = crc8(sys, 15);

  // 3) Stage/apply configs
  ESP_LOGD(TAG, "Staging RFID config...");
  if (!stage_apply_cfg(0x01, rfid, sizeof(rfid))) {
    this->mark_failed();
    return;
  }
  ESP_LOGD(TAG, "Staging SYS config...");
  if (!stage_apply_cfg(0x02, sys, sizeof(sys))) {
    this->mark_failed();
    return;
  }

  // 4) ENABLE_OP → wait OP_READY
  ESP_LOGD(TAG, "Enabling operation");
  write_u8(REG_CONTROL0, CTL_ENABLE_OP);
  if (!wait_status(ST_OP_READY, 1000)) {
    ESP_LOGE(TAG, "OP_READY timeout");
    this->mark_failed();
    return;
  }

  ESP_LOGI(TAG, "✅ Base comms online (OPERATION READY)");
  state_ = State::READY;
  last_ms_ = millis();
}

void PicoPanelBase::loop() {
  if (state_ != State::READY) return;

  // poll once per second: keepalive + basic status counters
  uint32_t now = millis();
  if (now - last_ms_ >= 1000) {
    write_u8(REG_CONTROL0, CTL_KEEPALIVE);

    uint8_t alive = 0, rx = 0, tx = 0;
    read_u8(REG_ALIVE, alive);
    read_u8(REG_RX_SEQ, rx);
    read_u8(REG_TX_SEQ, tx);

    uint8_t tbuf[4] = {0};
    read_block(REG_UPTIME_S, tbuf, 4);
    uint32_t up = (uint32_t)tbuf[0] | ((uint32_t)tbuf[1] << 8) |
                  ((uint32_t)tbuf[2] << 16) | ((uint32_t)tbuf[3] << 24);

    ESP_LOGV(TAG, "Alive=%u Uptime=%lus RX_SEQ=%u TX_SEQ=%u",
             alive, (unsigned long) up, rx, tx);

    last_ms_ = now;
  }
}

void PicoPanelBase::dump_config() {
  ESP_LOGCONFIG(TAG, "PicoPanelBase:");
  LOG_I2C_DEVICE(this);  // nice address/log dump from ESPHome I2C core :contentReference[oaicite:6]{index=6}
}

}  // namespace pico_panel_base
}  // namespace esphome
