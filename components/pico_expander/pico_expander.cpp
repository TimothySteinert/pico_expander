// pico_expander.cpp
#include "pico_expander.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pico_expander {

static const char *const TAG = "pico_expander";

// Register map:
// 0x00 = Red
// 0x01 = Green
// 0x02 = Blue

void PicoExpanderComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up PicoExpander (RGB) at 0x%02X ...", this->address_);
}

void PicoExpanderComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "PicoExpander RGB:");
  LOG_I2C_DEVICE(this)
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with PicoExpander failed!");
  }
}

void PicoExpanderComponent::write_rgb(uint8_t r, uint8_t g, uint8_t b) {
  uint8_t buf[3] = {r, g, b};
  if (this->write_register(0x00, buf, 3) != i2c::ERROR_OK) {
    this->status_set_warning();
  }
}

}  // namespace pico_expander
}  // namespace esphome
