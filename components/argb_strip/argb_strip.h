#pragma once
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/gpio.h"
#include "esphome/components/output/float_output.h"

#include <map>
#include <vector>
#include <string>
#include <cmath>

#ifdef USE_ESP32
#include "esp_idf_version.h"

#if ESP_IDF_VERSION_MAJOR < 5
#error "argb_strip 0.3.1-rmt requires ESP-IDF v5+ (new RMT APIs)."
#endif

#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"

namespace esphome {
namespace argb_strip {

static const char *const ARGB_STRIP_VERSION = "0.3.1-rmt";

class ARGBStripComponent : public Component {
 public:
  void set_pin(GPIOPin *pin) { pin_ = pin; }
  void set_raw_gpio(int raw) { raw_gpio_ = raw; }
  void set_num_leds(uint16_t n) { num_leds_ = n; }
  void set_global_brightness(float b) { global_brightness_ = b; }
  void set_gamma_enabled(bool g) { gamma_enabled_ = g; }

  void add_group(const std::string &name, const std::vector<int> &leds) { groups_[name] = leds; }
  const std::vector<int> *get_group(const std::string &name) const {
    auto it = groups_.find(name);
    if (it == groups_.end()) return nullptr;
    return &it->second;
  }

  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  void update_group_channel(const std::string &group, uint8_t channel, uint8_t value);
  void commit() { send_frame_(); }

 protected:
  GPIOPin *pin_{nullptr};
  int raw_gpio_{-1};
  uint16_t num_leds_{0};

  std::map<std::string, std::vector<int>> groups_;

  // Internal GRB buffer (WS2812 expects GRB bit order; we store already in GRB to avoid per-frame shuffle)
  std::vector<uint8_t> grb_; // size = num_leds_ * 3

  // Brightness / gamma
  float global_brightness_{1.0f};
  bool gamma_enabled_{false};
  uint8_t gamma_lut_[256];
  void build_gamma_();
  inline uint8_t postprocess(uint8_t v) const {
    if (global_brightness_ < 0.999f)
      v = (uint8_t)(v * global_brightness_ + 0.5f);
    if (gamma_enabled_)
      v = gamma_lut_[v];
    return v;
  }

  // New RMT resources
  rmt_channel_handle_t tx_channel_{nullptr};
  rmt_encoder_handle_t bytes_encoder_{nullptr};
  rmt_encoder_handle_t copy_encoder_{nullptr};
  bool rmt_ready_{false};

  // Reset (latch) symbol
  rmt_symbol_word_t reset_symbol_{};

  // Transmission config (loop_count=0)
  rmt_transmit_config_t tx_cfg_{};

  void init_rmt_();
  void send_frame_();
};

class ARGBStripOutput : public output::FloatOutput, public Component {
 public:
  void set_parent(ARGBStripComponent *p) { parent_ = p; }
  void set_group_name(const std::string &g) { group_ = g; }
  void set_channel(uint8_t ch) { channel_ = ch; }

  void setup() override {}
  void dump_config() override {}

 protected:
  void write_state(float state) override;

  ARGBStripComponent *parent_{nullptr};
  std::string group_;
  uint8_t channel_{0}; // 0=R 1=G 2=B (logical) mapped to GRB storage indices: G=0,R=1,B=2
};

}  // namespace argb_strip
}  // namespace esphome

#endif  // USE_ESP32
