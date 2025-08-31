#include "pico_expander.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pico_expander {

static const char *const TAG = "pico_expander";

void PicoExpanderComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up PicoExpander at 0x%02X ...", this->address_);
  // Nothing else to do; writes are one-byte per register.
}

void PicoExpanderComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "PicoExpander (3-channel byte registers for RGB):");
  LOG_I2C_DEVICE(this)
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with PicoExpander failed!");
  }
}

void PicoExpanderComponent::write_value(uint8_t channel, uint8_t value) {
  if (channel > 2) return;  // Only 3 registers: 0,1,2
  const uint8_t reg = channel;  // 0=R,1=G,2=B
  if (this->write_register(reg, &value, 1) != i2c::ERROR_OK) {
    this->status_set_warning();
    ESP_LOGW(TAG, "I2C write failed (reg=%u, val=0x%02X)", reg, value);
  } else {
    this->status_clear_warning();
    ESP_LOGV(TAG, "I2C write ok (reg=%u, val=0x%02X)", reg, value);
  }
}

}  // namespace pico_expander
}  // namespace esphome
