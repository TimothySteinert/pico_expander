#include "pico_expander.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pico_expander {

static const char *const TAG = "pico_expander";

void PicoExpanderComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up PicoExpander at 0x%02X ...", this->address_);
}

void PicoExpanderComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "PicoExpander (I2C register LED driver)");
  LOG_I2C_DEVICE(this)
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with PicoExpander failed!");
  }
}

void PicoExpanderComponent::write_value(uint8_t channel, uint8_t value) {
  const uint8_t reg = channel;  // channel is the raw register address (0x30–0x3E)
  if (this->write_register(reg, &value, 1) != i2c::ERROR_OK) {
    this->status_set_warning();
    ESP_LOGW(TAG, "I2C write failed (reg=0x%02X, val=0x%02X)", reg, value);
  } else {
    this->status_clear_warning();
    ESP_LOGV(TAG, "I2C write ok (reg=0x%02X, val=0x%02X)", reg, value);
  }
}

void PicoExpanderGPIOPin::digital_write(bool value) {
  if (!parent_) return;
  bool out = inverted_ ? !value : value;
  const uint8_t byte_val = out ? 0x11 : 0x00;
  parent_->write_value(channel_, byte_val);
  ESP_LOGV("pico_expander.gpio", "Write reg=0x%02X value=0x%02X (flags=%u)", channel_, byte_val, static_cast<unsigned>(flags_));
}


}  // namespace pico_expander
}  // namespace esphome
