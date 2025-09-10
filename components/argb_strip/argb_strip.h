#pragma once
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/gpio.h"
#include "esphome/components/output/float_output.h"
#include <map>
#include <vector>
#include <string>
#include <cstdint>

#ifdef USE_ESP32
#include "esp_idf_version.h"
#if ESP_IDF_VERSION_MAJOR < 5
#error "argb_strip 0.5.1-internal requires ESP-IDF v5+ (new RMT APIs)."
#endif

#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"

namespace esphome {
namespace argb_strip {

static const char *const ARGB_STRIP_VERSION = "0.5.1-internal";

enum class StripMode : uint8_t {
  NORMAL = 0,
  RFID_PROGRAM = 1
};

enum class ArmSelectMode : uint8_t {
  NONE = 0,
  AWAY,
  HOME,
  DISARM,
  NIGHT,
  VACATION,
  BYPASS
};

class ARGBStripComponent : public Component {
 public:
  void set_pin(GPIOPin *pin) { pin_ = pin; }
  void set_raw_gpio(int raw) { raw_gpio_ = raw; }
  void set_num_leds(uint16_t n) { num_leds_ = n; }

  void add_group(const std::string &name, const std::vector<int> &leds, float max_brightness);
  const std::vector<int> *get_group(const std::string &name) const;
  float get_group_max(const std::string &name) const;

  // Called by output channels in NORMAL mode
  void update_group_channel(const std::string &group, uint8_t channel, uint8_t value);

  // Internal control (call from lambdas / internal selects)
  void enable_rfid_mode();
  void disable_rfid_mode();
  void set_arm_select_mode(ArmSelectMode m);
  void set_arm_select_mode_by_name(const char *name); // helper

  // Lifecycle
  void setup() override;
  void dump_config() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  // Accessors if needed by other components later
  StripMode strip_mode() const { return strip_mode_; }
  ArmSelectMode arm_select_mode() const { return arm_select_mode_; }

 protected:
  GPIOPin *pin_{nullptr};
  int raw_gpio_{-1};
  uint16_t num_leds_{0};

  std::map<std::string, std::vector<int>> groups_;
  std::map<std::string, float> group_max_;

  std::vector<uint8_t> base_raw_grb_;
  std::vector<uint8_t> working_grb_;
  std::vector<uint8_t> last_sent_grb_;

  StripMode strip_mode_{StripMode::NORMAL};
  ArmSelectMode arm_select_mode_{ArmSelectMode::NONE};

  // Rainbow
  uint32_t rainbow_start_ms_{0};
  static constexpr uint32_t RAINBOW_CYCLE_MS = 8000;

  // Flash overlay (arm select)
  static constexpr uint32_t FLASH_ON_MS = 400;
  static constexpr uint32_t FLASH_OFF_MS = 400;

  // RMT
  rmt_channel_handle_t tx_channel_{nullptr};
  rmt_encoder_handle_t bytes_encoder_{nullptr};
  rmt_encoder_handle_t copy_encoder_{nullptr};
  bool rmt_ready_{false};
  rmt_symbol_word_t reset_symbol_{};
  rmt_transmit_config_t tx_cfg_{};

  bool frame_dirty_{false};
  uint32_t last_anim_eval_{0};
  static constexpr uint32_t ANIM_TICK_MS = 40;

  void init_rmt_();
  void mark_dirty_() { frame_dirty_ = true; }

  // Recomposition
  void recomposite_();
  void build_normal_base_();
  void build_rainbow_frame_();
  void apply_arm_select_overlay_(); // only if NOT in RFID mode
  void send_frame_();

  uint8_t scale_group_value_(const std::string &group, uint8_t v) const;
  int get_arm_select_led_index_() const;

  void hsv_to_grb_(float h, float s, float v, uint8_t &g, uint8_t &r, uint8_t &b) const;
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
  uint8_t channel_{0}; // 0=R 1=G 2=B
};

}  // namespace argb_strip
}  // namespace esphome

#endif  // USE_ESP32
