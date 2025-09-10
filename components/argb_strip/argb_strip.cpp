#include "argb_strip.h"
#include "esphome/core/log.h"

#ifdef USE_ESP32

namespace esphome {
namespace argb_strip {

static const char *const TAG = "argb_strip";

// WS2812 (800kHz) timing expressed in 25 ns ticks if we set resolution to 40MHz (we will)
static const uint32_t T0H = 16;  // 0.40us
static const uint32_t T0L = 34;  // 0.85us
static const uint32_t T1H = 32;  // 0.80us
static const uint32_t T1L = 18;  // 0.45us

void ARGBStripComponent::build_gamma_() {
  if (!gamma_enabled_) return;
  for (int i = 0; i < 256; i++) {
    float x = (float) i / 255.0f;
    float y = powf(x, 2.2f);
    gamma_lut_[i] = (uint8_t)(y * 255.0f + 0.5f);
  }
}

void ARGBStripComponent::setup() {
  ESP_LOGCONFIG(TAG, "ARGB Strip v%s (modern RMT) LEDs=%u", ARGB_STRIP_VERSION, num_leds_);
  if (!pin_ || raw_gpio_ < 0) {
    ESP_LOGE(TAG, "Invalid pin configuration");
    this->mark_failed();
    return;
  }

  pin_->setup();
  pin_->digital_write(false);

  grb_.assign(num_leds_ * 3, 0);
  build_gamma_();
  init_rmt_();
  if (!rmt_ready_) {
    ESP_LOGE(TAG, "RMT init failed");
    this->mark_failed();
    return;
  }

  // Prepare reset symbol ( > 50us low ) : duration0 * tick(25ns) = 50us => 50us / 25ns = 2000
  reset_symbol_.level0 = 0;
  reset_symbol_.duration0 = 2000; // 50us
  reset_symbol_.level1 = 0;
  reset_symbol_.duration1 = 0;

  send_frame_(); // Clear
}

void ARGBStripComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "ARGB Strip:");
  LOG_PIN("  Pin: ", pin_);
  ESP_LOGCONFIG(TAG, "  Raw GPIO: %d", raw_gpio_);
  ESP_LOGCONFIG(TAG, "  LEDs: %u", num_leds_);
  ESP_LOGCONFIG(TAG, "  RMT Ready: %s", rmt_ready_ ? "YES" : "NO");
  ESP_LOGCONFIG(TAG, "  Global Brightness: %.2f", global_brightness_);
  ESP_LOGCONFIG(TAG, "  Gamma: %s", gamma_enabled_ ? "ENABLED" : "DISABLED");
  if (!groups_.empty()) {
    ESP_LOGCONFIG(TAG, "  Groups:");
    for (auto &kv : groups_) {
      ESP_LOGCONFIG(TAG, "    %s -> %u pixel(s)", kv.first.c_str(), (unsigned) kv.second.size());
    }
  }
}

void ARGBStripComponent::init_rmt_() {
  // Create TX channel
  rmt_tx_channel_config_t ch_cfg{};
  ch_cfg.gpio_num = (gpio_num_t) raw_gpio_;
  ch_cfg.clk_src = RMT_CLK_SRC_DEFAULT;
  ch_cfg.resolution_hz = 40'000'000; // 40 MHz -> 25 ns tick
  ch_cfg.mem_block_symbols = 64;     // enough; driver can queue
  ch_cfg.trans_queue_depth = 4;

  if (rmt_new_tx_channel(&ch_cfg, &tx_channel_) != ESP_OK) {
    ESP_LOGE(TAG, "rmt_new_tx_channel failed");
    return;
  }

  // Configure bytes encoder with WS2812 bit timings
  rmt_bytes_encoder_config_t bcfg{};
  bcfg.bit0.level0 = 1;
  bcfg.bit0.duration0 = T0H;
  bcfg.bit0.level1 = 0;
  bcfg.bit0.duration1 = T0L;
  bcfg.bit1.level0 = 1;
  bcfg.bit1.duration0 = T1H;
  bcfg.bit1.level1 = 0;
  bcfg.bit1.duration1 = T1L;
  bcfg.flags.msb_first = 1;

  if (rmt_new_bytes_encoder(&bcfg, &bytes_encoder_) != ESP_OK) {
    ESP_LOGE(TAG, "rmt_new_bytes_encoder failed");
    return;
  }

  // Copy encoder for reset symbol
  rmt_copy_encoder_config_t cpy_cfg{};
  if (rmt_new_copy_encoder(&cpy_cfg, &copy_encoder_) != ESP_OK) {
    ESP_LOGE(TAG, "rmt_new_copy_encoder failed");
    return;
  }

  if (rmt_enable(tx_channel_) != ESP_OK) {
    ESP_LOGE(TAG, "rmt_enable failed");
    return;
  }

  tx_cfg_.loop_count = 0;
  rmt_ready_ = true;
  ESP_LOGD(TAG, "RMT channel initialized on GPIO %d", raw_gpio_);
}

void ARGBStripComponent::update_group_channel(const std::string &group, uint8_t channel, uint8_t value) {
  if (!rmt_ready_) return;
  auto g = this->get_group(group);
  if (!g) {
    ESP_LOGW(TAG, "Group '%s' not found", group.c_str());
    return;
  }

  uint8_t adj = postprocess(value);

  // Stored as GRB (indexes: G=0, R=1, B=2)
  for (int led : *g) {
    if (led < 0 || (uint16_t) led >= num_leds_) continue;
    uint32_t base = led * 3;
    switch (channel) {
      case 0: grb_[base + 1] = adj; break; // R
      case 1: grb_[base + 0] = adj; break; // G
      case 2: grb_[base + 2] = adj; break; // B
    }
  }
  send_frame_();
}

void ARGBStripComponent::send_frame_() {
  if (!rmt_ready_ || num_leds_ == 0) return;

  // Send pixel bytes
  esp_err_t err = rmt_transmit(tx_channel_, bytes_encoder_, grb_.data(), grb_.size(), &tx_cfg_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "rmt_transmit (data) failed err=0x%X", err);
    return;
  }
  err = rmt_tx_wait_all_done(tx_channel_, pdMS_TO_TICKS(20));
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "rmt_tx_wait_all_done (data) err=0x%X", err);
    return;
  }

  // Send reset latch
  err = rmt_transmit(tx_channel_, copy_encoder_, &reset_symbol_, sizeof(reset_symbol_), &tx_cfg_);
  if (err == ESP_OK) {
    rmt_tx_wait_all_done(tx_channel_, pdMS_TO_TICKS(10));
  } else {
    ESP_LOGE(TAG, "rmt_transmit (reset) failed err=0x%X", err);
  }
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
