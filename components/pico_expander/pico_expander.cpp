#include "pico_expander.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pico_expander {

static const char *const TAG = "pico_expander";

// Register map:
// 0x00 = GPIO0 mode (0x20=input, 0x3F=output)
// 0x01 = GPIO1 mode (0x20=input, 0x3F=output)
// 0x02 = GPIO0 value (0x00 or 0x11)
// 0x03 = GPIO1 value (0x00 or 0x11)

void PicoExpanderComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up PicoExpander at 0x%02X ...", this->address_);
}

void PicoExpanderComponent::loop() {
  // Nothing fancy, will just read/write registers when asked
}

void PicoExpanderComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "PicoExpander:");
  LOG_I2C_DEVICE(this)
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with PicoExpander failed!");
  }
}

bool PicoExpanderComponent::digital_read(uint8_t pin) {
  if (pin > 1) return false;

  uint8_t value;
  uint8_t reg = (pin == 0) ? 0x02 : 0x03;
  if (this->read_register(reg, &value, 1) != i2c::ERROR_OK) {
    this->status_set_warning();
    return false;
  }
  this->last_state_[pin] = value;
  return value == 0x11;
}

void PicoExpanderComponent::digital_write(uint8_t pin, bool value) {
  if (pin > 1) return;

  uint8_t reg = (pin == 0) ? 0x02 : 0x03;
  uint8_t data = value ? 0x11 : 0x00;
  if (this->write_register(reg, &data, 1) != i2c::ERROR_OK) {
    this->status_set_warning();
  } else {
    this->last_state_[pin] = data;
  }
}

void PicoExpanderComponent::pin_mode(uint8_t pin, gpio::Flags flags) {
  if (pin > 1) return;

  uint8_t reg = (pin == 0) ? 0x00 : 0x01;
  uint8_t data;

  if (flags == gpio::FLAG_INPUT) {
    data = 0x20;
  } else {
    data = 0x3F;
  }

  if (this->write_register(reg, &data, 1) != i2c::ERROR_OK) {
    this->status_set_warning();
  }
}

void PicoExpanderGPIOPin::setup() {
  pin_mode(flags_);
  this->setup_ = true;
}

void PicoExpanderGPIOPin::pin_mode(gpio::Flags flags) {
  this->parent_->pin_mode(this->pin_, flags);
}

bool PicoExpanderGPIOPin::digital_read() {
  return this->parent_->digital_read(this->pin_) != this->inverted_;
}

void PicoExpanderGPIOPin::digital_write(bool value) {
  if (this->setup_)
    this->parent_->digital_write(this->pin_, value != this->inverted_);
}

std::string PicoExpanderGPIOPin::dump_summary() const {
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "GPIO %u via PicoExpander", pin_);
  return buffer;
}

}  // namespace pico_expander
}  // namespace esphome
