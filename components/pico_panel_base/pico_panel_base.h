#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace pico_panel_base {

using esphome::i2c::I2CDevice;
using esphome::i2c::ErrorCode;

class PicoPanelBase : public Component, public I2CDevice {
 public:
  // ---- Component API ----
  void setup() override;
  void loop() override;
  void dump_config() override;

  // simple setters from YAML
  void set_rfid_key_id(uint8_t v) { rfid_key_id_ = v; }
  void set_rfid_flags(uint8_t v)  { rfid_flags_  = v; }
  void set_sys_options(uint8_t v) { sys_options_ = v; }

 protected:
  // ---- Register map (subset) ----
  static constexpr uint8_t REG_DEVICE_ID  = 0x00;
  static constexpr uint8_t REG_VERSION    = 0x01;
  static constexpr uint8_t REG_STATUS0    = 0x02;
  static constexpr uint8_t REG_CONTROL0   = 0x03;
  static constexpr uint8_t REG_ERROR      = 0x04;
  static constexpr uint8_t REG_TX_SEQ     = 0x05;
  static constexpr uint8_t REG_RX_SEQ     = 0x06;
  static constexpr uint8_t REG_ALIVE      = 0x07;
  static constexpr uint8_t REG_UPTIME_S   = 0x08; // 0x08..0x0B

  static constexpr uint8_t REG_CFG_SEL    = 0x20;
  static constexpr uint8_t REG_CFG_LEN    = 0x21;
  static constexpr uint8_t REG_CFG_BASE   = 0x22; // window (<=64 bytes)

  // STATUS0 bits
  static constexpr uint8_t ST_READY       = 1u << 0;
  static constexpr uint8_t ST_CFG_STAGED  = 1u << 1;
  static constexpr uint8_t ST_CONFIGURED  = 1u << 2;
  static constexpr uint8_t ST_ERROR       = 1u << 3;
  static constexpr uint8_t ST_OP_READY    = 1u << 5;

  // CONTROL0 (write-1, self-clear)
  static constexpr uint8_t CTL_SOFT_RESET   = 1u << 0;
  static constexpr uint8_t CTL_KEEPALIVE    = 1u << 1;
  static constexpr uint8_t CTL_START_CONFIG = 1u << 2;
  static constexpr uint8_t CTL_APPLY_CONFIG = 1u << 3;
  static constexpr uint8_t CTL_ENABLE_OP    = 1u << 4;
  static constexpr uint8_t CTL_CLEAR_ERROR  = 1u << 5;

  // ---- Internal helpers ----
  static uint8_t crc8(const uint8_t *data, size_t len);
  bool read_u8(uint8_t reg, uint8_t &val);
  bool write_u8(uint8_t reg, uint8_t val);
  bool read_block(uint8_t reg, uint8_t *buf, size_t len);
  bool write_block(uint8_t reg, const uint8_t *buf, size_t len);

  bool wait_status(uint8_t bit_mask, uint32_t timeout_ms);
  bool stage_apply_cfg(uint8_t cfg_sel, const uint8_t *data, uint8_t len_including_crc);

  // ---- State machine ----
  enum class State : uint8_t {
    INIT, RESET, WAIT_READY, STAGE_RFID, STAGE_SYS, ENABLE_OP, WAIT_OP_READY, READY
  } state_{State::INIT};

  uint32_t last_ms_{0};

  // config bytes (kept tiny for the base layer)
  uint8_t rfid_key_id_{0x10};
  uint8_t rfid_flags_{0xA0};
  uint8_t sys_options_{0x55};
};

}  // namespace pico_panel_base
}  // namespace esphome
