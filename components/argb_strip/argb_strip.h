#pragma once
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/gpio.h"
#include "esphome/components/output/float_output.h"

#include <map>
#include <vector>
#include <string>

#ifdef USE_ESP32
#include "esp_idf_version.h"

#if ESP_IDF_VERSION_MAJOR < 5
#error "argb_strip 0.3.2-groups requires ESP-IDF v5+ (new RMT APIs)."
#endif

#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"

namespace esphome {
namespace argb_strip {

static const char *const ARGB_STRIP_VERSION = "0.3.2-groups";

class ARGBStripComponent : public Component {
 public:
  void set_pin(GPIOPin *pin) { pin_ = pin; }
  void set_raw_gpio(int raw) { raw_gpio_ = raw; }
  void set_num_leds(uint16_t n) { num_leds_ = n; }
  void set_global_brightness(float b) { global_brightness_ = b; }

  void add_group(const std::string &name, const std::vector<int> &leds, float max_brightness);
  const std::vector<int> *get_group(const std::string &name) const;
  float get_group_max(const std::string &name) const;

  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  void update_group_channel(const std::string &group, uint8_t channel, uint8_t value);
  void commit() { send_frame_(); }

 protected:
  GPIOPin *pin_{nullptr};
  int raw_gpio_{-1};
  uint16_t num_leds_{0};

  // Per-group data
  std::map<std::string, std::vector<int>> groups_;
  std::map<std::string, float> group_max_;  // 0.0 - 1.0

  // Internal GRB buffer
  std::vector<uint8_t> grb_; // G,R,B ordering per LED (WS2812 expects GRB bits)

  // Global brightness (optional final scale)
  float global_brightness_{1.0f};

  // RMT resources
  rmt_channel_handle_t tx_channel_{nullptr};
  rmt_encoder_handle_t bytes_encoder_{nullptr};
  rmt_encoder_handle_t copy_encoder_{nullptr};
  bool rmt_ready_{false};
  rmt_symbol_word_t reset_symbol_{};
  rmt_transmit_config_t tx_cfg_{};

  void init_rmt_();
  void send_frame_();
  uint8_t apply_scaling_(const std::string &group, uint8_t base_value) const;
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
  uint8_t channel_{0}; // logical: 0=R 1=G 2=B
};

}  // namespace argb_strip
}  // namespace esphome

#endif  // USE_ESP32
