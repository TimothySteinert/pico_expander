#include "argb_strip.h"
#include "esphome/core/log.h"

#ifdef USE_ESP32
#include "driver/rmt.h"

namespace esphome {
namespace argb_strip {

static const char *const TAG = "argb_strip";

// Timings (legacy RMT) for WS2812 at 800kHz using 40MHz source (clk_div=2 => 25ns ticks)
static const uint16_t T0H = 16;  // 0.4us
static const uint16_t T0L = 34;  // 0.85us
static const uint16_t T1H = 32;  // 0.8us
static const uint16_t T1L = 18;  // 0.45us

int ARGBStripComponent::resolve_gpio_num_() const {
  if (pin_ == nullptr) return -1;
  // Newer ESPHome versions usually provide raw pin via get_pin()
  // If that ever changes, this is where you'd adapt.
#if defined(USE_GPIO)
  // Guard: only compile call if symbol is available
  // (If compilation still fails, inspect esphome/core/gpio*.h for the correct accessor)
  return pin_->get_pin();
#else
  return -1;
#endif
}

void ARGBStripComponent::setup() {
  ESP_LOGCONFIG(TAG, "Initializing ARGB strip: %u LEDs", num_leds_);
  if (pin_ == nullptr) {
    ESP_LOGE(TAG, "No pin configured");
    this->mark_failed();
    return;
  }
  pin_->setup();
  pin_->digital_write(false);
  buffer_.assign(num_leds_ * 3, 0);
  init_rmt_();
  send_();  // clear
}

void ARGBStripComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "ARGB Strip:");
  LOG_PIN("  Pin: ", pin_);
  ESP_LOGCONFIG(TAG, "  LEDs: %u", num_leds_);
  ESP_LOGCONFIG(TAG, "  Groups: %u", (unsigned) groups_.size());
  for (auto &kv : groups_) {
    ESP_LOGV(TAG, "    %s -> %u LEDs", kv.first.c_str(), (unsigned) kv.second.size());
  }
  ESP_LOGCONFIG(TAG, "  RMT driver: %s", rmt_ok_ ? "OK" : "FAILED");
}

void ARGBStripComponent::init_rmt_() {
  int gpio_num = resolve_gpio_num_();
  if (gpio_num < 0) {
    ESP_LOGE(TAG, "Could not resolve GPIO number for RMT");
    return;
  }

  rmt_channel_ = 0;  // fixed channel for now

  rmt_config_t cfg{};
  cfg.rmt_mode = RMT_MODE_TX;
  cfg.channel = (rmt_channel_t) rmt_channel_;
  cfg.gpio_num = (gpio_num_t) gpio_num;
  cfg.mem_block_num = 1;
  cfg.clk_div = 2;  // 80MHz / 2 = 40MHz -> 25ns tick
  cfg.tx_config.loop_en = false;
  cfg.tx_config.carrier_en = false;
  cfg.tx_config.idle_output_en = true;
  cfg.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;

  if (rmt_config(&cfg) != ESP_OK) {
    ESP_LOGE(TAG, "rmt_config failed");
    return;
  }
  if (rmt_driver_install(cfg.channel, 0, 0) != ESP_OK) {
    ESP_LOGE(TAG, "rmt_driver_install failed");
    return;
  }
  rmt_ok_ = true;
}

void ARGBStripComponent::update_group_channel(const std::string &group, uint8_t channel, uint8_t value) {
  auto g = this->get_group(group);
  if (g == nullptr) return;

  for (int led : *g) {
    if (led < 0 || (uint16_t) led >= num_leds_) continue;
    uint32_t base = led * 3;
    // buffer_ layout GRB
    switch (channel) {
      case 0: buffer_[base + 1] = value; break;  // red
      case 1: buffer_[base + 0] = value; break;  // green
      case 2: buffer_[base + 2] = value; break;  // blue
    }
  }
  send_();
}

void ARGBStripComponent::send_() {
  if (!rmt_ok_ || num_leds_ == 0) return;

  const size_t bit_count = num_leds_ * 24;
  std::vector<rmt_item32_t> items;
  items.reserve(bit_count);

  for (uint16_t led = 0; led < num_leds_; led++) {
    uint32_t base = led * 3;
    uint8_t g = buffer_[base + 0];
    uint8_t r = buffer_[base + 1];
    uint8_t b = buffer_[base + 2];
    uint8_t colors[3] = {g, r, b};

    for (uint8_t c = 0; c < 3; c++) {
      uint8_t val = colors[c];
      for (int bit = 7; bit >= 0; bit--) {
        bool one = (val >> bit) & 0x01;
        rmt_item32_t it{};
        if (one) {
          it.level0 = 1; it.duration0 = T1H;
          it.level1 = 0; it.duration1 = T1L;
        } else {
          it.level0 = 1; it.duration0 = T0H;
          it.level1 = 0; it.duration1 = T0L;
        }
        items.push_back(it);
      }
    }
  }

  rmt_write_items((rmt_channel_t) rmt_channel_, items.data(), items.size(), true);
  // Latch/reset > 50us low
  delay(1);
}

void ARGBStripOutput::write_state(float state) {
  if (!parent_) return;
  if (state < 0.f) state = 0.f;
  if (state > 1.f) state = 1.f;
  uint8_t v = static_cast<uint8_t>(state * 255.0f + 0.5f);
  parent_->update_group_channel(group_, channel_, v);
}

}  // namespace argb_strip
}  // namespace esphome

#endif  // USE_ESP32
