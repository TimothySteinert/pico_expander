#include "argb_strip.h"
#include "esphome/core/log.h"

#ifdef USE_ESP32
#include "esp_idf_version.h"

namespace esphome {
namespace argb_strip {

static const char *const TAG = "argb_strip";

void ARGBStripComponent::build_gamma_() {
  if (!gamma_enabled_) return;
  for (int i = 0; i < 256; i++) {
    float x = (float) i / 255.0f;
    float g = powf(x, 2.2f);
    gamma_lut_[i] = (uint8_t) (g * 255.0f + 0.5f);
  }
}

void ARGBStripComponent::setup() {
  ESP_LOGCONFIG(TAG, "ARGB Strip v%s (IDF led_strip) LEDs=%u", ARGB_STRIP_VERSION, num_leds_);
  if (!pin_ || raw_gpio_ < 0) {
    ESP_LOGE(TAG, "Pin not configured correctly");
    this->mark_failed();
    return;
  }
  pin_->setup();
  pin_->digital_write(false);

  rgb_.assign(num_leds_ * 3, 0);
  build_gamma_();
  init_driver_();
  if (!ready_) {
    ESP_LOGE(TAG, "Driver init failed");
    this->mark_failed();
    return;
  }
  send_();  // Clear
}

void ARGBStripComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "ARGB Strip:");
  LOG_PIN("  Pin Object: ", pin_);
  ESP_LOGCONFIG(TAG, "  Raw GPIO: %d", raw_gpio_);
  ESP_LOGCONFIG(TAG, "  LEDs: %u", num_leds_);
  ESP_LOGCONFIG(TAG, "  Driver Ready: %s", ready_ ? "YES" : "NO");
  ESP_LOGCONFIG(TAG, "  Global Brightness: %.2f", global_brightness_);
  ESP_LOGCONFIG(TAG, "  Gamma: %s", gamma_enabled_ ? "ENABLED" : "DISABLED");
  if (!groups_.empty()) {
    ESP_LOGCONFIG(TAG, "  Groups:");
    for (auto &kv : groups_) {
      ESP_LOGCONFIG(TAG, "    %s -> %u pixel(s)", kv.first.c_str(), (unsigned) kv.second.size());
    }
  }
}

void ARGBStripComponent::init_driver_() {
  // Configure the led_strip helper for WS2812 (GRB)
  led_strip_config_t strip_config = {
    .strip_gpio_num = (gpio_num_t) raw_gpio_,
    .max_leds = num_leds_,
    .led_model = LED_MODEL_WS2812,
    .color_component_format = LED_STRIP_COLOR_COMPONENT_FORMAT_GRB,
    .flags = {
      .invert_out = 0
    }
  };

  led_strip_rmt_config_t rmt_config = {
    .clk_src = RMT_CLK_SRC_DEFAULT,
    .resolution_hz = 10'000'000,  // 10 MHz is typical; driver handles timing. Could use 40MHz too.
    .mem_block_symbols = 0,       // auto
    .flags = {
      .with_dma = 0
    }
  };

  esp_err_t err = led_strip_new_rmt_device(&strip_config, &rmt_config, &strip_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "led_strip_new_rmt_device failed err=0x%X", err);
    return;
  }
  ready_ = true;
  ESP_LOGD(TAG, "led_strip driver initialized on GPIO %d", raw_gpio_);
}

void ARGBStripComponent::update_group_channel(const std::string &group, uint8_t channel, uint8_t value) {
  if (!ready_) return;
  auto g = this->get_group(group);
  if (!g) {
    ESP_LOGW(TAG, "Group '%s' not found", group.c_str());
    return;
  }
  uint8_t adj = post_(value);

  for (int led : *g) {
    if (led < 0 || (uint16_t) led >= num_leds_) continue;
    uint32_t base = led * 3;
    switch (channel) {
      case 0: rgb_[base + 0] = adj; break; // R
      case 1: rgb_[base + 1] = adj; break; // G
      case 2: rgb_[base + 2] = adj; break; // B
    }
  }
  send_();
}

void ARGBStripComponent::send_() {
  if (!ready_) return;

  // Push all pixels (driver expects R,G,B; color_component_format handles GRB output)
  for (uint16_t i = 0; i < num_leds_; i++) {
    uint32_t base = i * 3;
    uint8_t r = rgb_[base + 0];
    uint8_t g = rgb_[base + 1];
    uint8_t b = rgb_[base + 2];
    esp_err_t err = led_strip_set_pixel(strip_, i, r, g, b);
    if (err != ESP_OK) {
      ESP_LOGW(TAG, "led_strip_set_pixel failed idx=%u err=0x%X", i, err);
      return;
    }
  }
  esp_err_t err = led_strip_refresh(strip_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "led_strip_refresh failed err=0x%X", err);
  }
}

void ARGBStripOutput::write_state(float state) {
  if (!parent_) return;
  if (state < 0.f) state = 0.f;
  if (state > 1.f) state = 1.f;
  uint8_t v = (uint8_t) (state * 255.0f + 0.5f);
  parent_->update_group_channel(group_, channel_, v);
}

}  // namespace argb_strip
}  // namespace esphome

#endif  // USE_ESP32
