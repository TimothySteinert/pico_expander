#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include "driver/rmt.h"
#include "driver/gpio.h"

namespace esphome {
namespace argb_strip {

class ARGBStripComponent : public Component {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  void set_pin(GPIOPin *pin) { pin_ = pin; }
  void set_num_leds(uint8_t num_leds) { num_leds_ = num_leds; }
  
  // Set RGB for a specific LED (0-based index)
  void set_led_rgb(uint8_t led_index, uint8_t red, uint8_t green, uint8_t blue);
  
  // Update the physical strip
  void show();

 protected:
  GPIOPin *pin_;
  uint8_t num_leds_{6};
  rmt_channel_t rmt_channel_{RMT_CHANNEL_0};
  
  // RGB buffer: [LED0_R, LED0_G, LED0_B, LED1_R, LED1_G, LED1_B, ...]
  uint8_t *led_buffer_{nullptr};
  
  void init_rmt();
  void send_data();
};

}  // namespace argb_strip
}  // namespace esphome
