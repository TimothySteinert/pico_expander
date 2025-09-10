#pragma once
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/output/float_output.h"
#include "esphome/core/gpio.h"   // Ensure full GPIOPin declaration

#include <map>
#include <vector>
#include <string>

#ifdef USE_ESP32

namespace esphome {
namespace argb_strip {

class ARGBStripComponent : public Component {
 public:
  void set_pin(GPIOPin *pin) { pin_ = pin; }
  void set_num_leds(uint16_t n) { num_leds_ = n; }
  void add_group(const std::string &name, const std::vector<int> &leds) { groups_[name] = leds; }

  const std::vector<int> *get_group(const std::string &name) const {
    auto it = groups_.find(name);
    if (it == groups_.end()) return nullptr;
    return &it->second;
  }

  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  // Update one color channel (0=R,1=G,2=B) across all LEDs in a group.
  void update_group_channel(const std::string &group, uint8_t channel, uint8_t value);

 protected:
  GPIOPin *pin_{nullptr};
  uint16_t num_leds_{0};
  std::map<std::string, std::vector<int>> groups_;
  std::vector<uint8_t> buffer_;  // GRB per LED

  // RMT state
  bool rmt_ok_{false};
  int rmt_channel_{0};

  void init_rmt_();
  void send_();
  int resolve_gpio_num_() const;  // helper
};

class ARGBStripOutput : public output::FloatOutput, public Component {
 public:
  void set_parent(ARGBStripComponent *p) { parent_ = p; }
  void set_group_name(const std::string &g) { group_ = g; }
  void set_channel(uint8_t ch) { channel_ = ch; }

  void setup() override {}
  void dump_config() override {}

 protected:
  void write_state(float state) override;  // implemented in .cpp

  ARGBStripComponent *parent_{nullptr};
  std::string group_;
  uint8_t channel_{0};  // 0=R,1=G,2=B
};

}  // namespace argb_strip
}  // namespace esphome

#endif  // USE_ESP32
