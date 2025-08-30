#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace pico_panel_base {

using esphome::i2c::I2CDevice;

class PicoPanelBase : public Component, public I2CDevice {
 public:
  // Component API
  void setup() override;
  void loop() override;
  void dump_config() override;

 protected:
  // ---- Minimal register map ----
  static constexpr uint8_t REG_DEVICE_ID = 0x00;  // expect 0xA5
  static constexpr uint8_t REG_VERSION   = 0x01;
  static constexpr uint8_t REG_STATUS0   = 0x02;  // bit0 READY, bit3 ERROR, bit5 OP_READY
  static constexpr uint8_t REG_CONTROL0  = 0x03;  // write-1: SOFT_RESET, KEEPALIVE
  static constexpr uint8_t REG_ERROR     = 0x04;
  static constexpr uint8_t REG_ALIVE     = 0x07;

  // STATUS0 bits
  static constexpr uint8_t ST_READY    = 1u << 0;
  static constexpr uint8_t ST_ERROR    = 1u << 3;
  static constexpr uint8_t ST_OP_READY = 1u << 5;

  // CONTROL0 bits (write-1, self-clear)
  static constexpr uint8_t CTL_SOFT_RESET = 1u << 0;
  static constexpr uint8_t CTL_KEEPALIVE  = 1u << 1;

  // ---- Tiny helpers ----
  static uint8_t crc8(const uint8_t *data, size_t len);
  bool read_u8(uint8_t reg, uint8_t &val);
  bool write_u8(uint8_t reg, uint8_t val);

  bool wait_status(uint8_t bit_mask, uint32_t timeout_ms);

  // ---- State ----
  bool online_{false};
  uint32_t last_ms_{0};
};

}  // namespace pico_panel_base
}  // namespace esphome
