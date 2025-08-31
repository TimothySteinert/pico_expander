#include "pico_expander.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pico_expander {

static const char *const TAG = "pico_expander";

void PicoExpanderComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up PicoExpander RGB at 0x%02X ...", this->address_);
}

void PicoExpanderComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "PicoExpander RGB:");
  LOG_I2C_DEVICE(this)
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with PicoExpander failed!");
  }
}

void PicoExpanderComponent::write_value(uint8_t channel, uint8_t value) {
  if (channel > 2) return;  // only 3 channels
  if (this->write_register(channel, &value, 1) != i2c::ERROR_OK) {
    this->status_set_warning();
  }
}

}  // namespace pico_expander
}  // namespace esphome
