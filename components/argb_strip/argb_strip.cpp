// NOTE: This file is the 16c base plus ACTION mode additions (version 16d)
#include "argb_strip.h"
#include "esphome/core/log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <algorithm>
#include <cmath>

#ifndef pdMS_TO_TICKS
#define pdMS_TO_TICKS(ms) ((ms) / portTICK_PERIOD_MS)
#endif

#ifdef USE_ESP32
static int g_argb_instances = 0;

namespace esphome {
namespace argb_strip {

static const char *const TAG = "argb_strip";

static const uint32_t T0H = 16;
static const uint32_t T0L = 34;
static const uint32_t T1H = 32;
static const uint32_t T1L = 18;

// Group management
void ARGBStripComponent::add_group(const std::string &name, const std::vector<int> &leds, uint16_t cap) {
  groups_[name] = leds;
  group_caps_[name] = cap;
}
const std::vector<int> *ARGBStripComponent::get_group(const std::string &name) const {
  auto it = groups_.find(name);
  if (it == groups_.end()) return nullptr;
  return &it->second;
}
uint16_t ARGBStripComponent::get_group_cap(const std::string &name) const {
  auto it = group_caps_.find(name);
  if (it == group_caps_.end()) return 255;
  return it->second;
}

void ARGBStripComponent::set_scaling_mode(const std::string &m) {
  std::string mm = m;
  std::transform(mm.begin(), mm.end(), mm.begin(), ::tolower);
  if (mm == "clamp") scaling_mode_ = ScalingMode::CLAMP;
  else if (mm == "perceptual") scaling_mode_ = ScalingMode::PERCEPTUAL;
  else scaling_mode_ = ScalingMode::LINEAR;
}

// Lifecycle
void ARGBStripComponent::setup() {
  int inst = ++g_argb_instances;
  ESP_LOGI(TAG, "Setup instance #%d, version %s", inst, ARGB_STRIP_VERSION);

  if (!pin_ || raw_gpio_ < 0) {
    ESP_LOGE(TAG, "Invalid pin configuration");
    this->mark_failed();
    return;
  }
  pin_->setup();
  pin_->digital_write(false);

  base_raw_grb_.assign(num_leds_ * 3, 0);
  working_grb_.assign(num_leds_ * 3, 0);
  last_sent_grb_.assign(num_leds_ * 3, 255);

  init_rmt_();
  if (!rmt_ready_) {
    ESP_LOGE(TAG, "RMT init failed");
    this->mark_failed();
    return;
  }

  reset_symbol_.level0 = 0;
  reset_symbol_.duration0 = 2000;
  reset_symbol_.level1 = 0;
  reset_symbol_.duration1 = 0;

  rainbow_start_ms_ = millis();
  rfid_transition_ = RfidTransitionState::INACTIVE;
  build_gamma_lut_if_needed_();
  action_rainbow_start_ms_ = rainbow_start_ms_; // initialize
  mark_dirty_();
}

void ARGBStripComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "ARGB Strip v%s", ARGB_STRIP_VERSION);
  LOG_PIN("  Pin: ", pin_);
  ESP_LOGCONFIG(TAG, "  Raw GPIO: %d", raw_gpio_);
  ESP_LOGCONFIG(TAG, "  LEDs: %u", num_leds_);
  ESP_LOGCONFIG(TAG, "  Scaling Mode: %d", (int)scaling_mode_);
  ESP_LOGCONFIG(TAG, "  Perceptual Gamma: %.3f", perceptual_gamma_);
  ESP_LOGCONFIG(TAG, "  Rainbow Cycle (ms): %u", rainbow_cycle_ms_);
  ESP_LOGCONFIG(TAG, "  ACTION Cycle (ms): %u", ACTION_RAINBOW_CYCLE_MS);
  for (auto &kv : groups_) {
    ESP_LOGCONFIG(TAG, "    Group %s size=%u cap=%u",
                  kv.first.c_str(), (unsigned)kv.second.size(), (unsigned)get_group_cap(kv.first));
  }
}

void ARGBStripComponent::loop() {
  uint32_t now = millis();
  bool anim_tick = (now - last_anim_eval_) >= ANIM_TICK_MS;

  if (rfid_visual_active_()) {
    if (anim_tick) mark_dirty_();
    if (rfid_transition_ == RfidTransitionState::FADE_IN) {
      if (now - rfid_transition_start_ms_ >= RFID_FADE_MS) {
        rfid_transition_ = RfidTransitionState::ACTIVE;
        mark_dirty_();
      }
    } else if (rfid_transition_ == RfidTransitionState::FADE_OUT) {
      if (now - rfid_transition_start_ms_ >= RFID_FADE_MS) {
        finish_rfid_fade_out_();
      }
    }
  } else {
    if (arm_select_mode_ != ArmSelectMode::NONE && anim_tick) {
      mark_dirty_(); // ensures ACTION rainbow animates
    }
    if (arm_select_disable_pending_) {
      uint32_t phase = (now % (FLASH_ON_MS + FLASH_OFF_MS));
      bool in_off_phase = phase >= FLASH_ON_MS;
      if (in_off_phase) {
        finalize_arm_select_disable_();
      }
    }
  }

  if (anim_tick) last_anim_eval_ = now;

  if (frame_dirty_) {
    recomposite_();
    send_frame_();
    frame_dirty_ = false;
  }
}

// RMT init
void ARGBStripComponent::init_rmt_() {
  rmt_tx_channel_config_t ch_cfg{};
  ch_cfg.gpio_num = (gpio_num_t) raw_gpio_;
  ch_cfg.clk_src = RMT_CLK_SRC_DEFAULT;
  ch_cfg.resolution_hz = 40'000'000;
  ch_cfg.mem_block_symbols = 64;
  ch_cfg.trans_queue_depth = 4;

  if (rmt_new_tx_channel(&ch_cfg, &tx_channel_) != ESP_OK) return;

  rmt_bytes_encoder_config_t bcfg{};
  bcfg.bit0.level0 = 1; bcfg.bit0.duration0 = T0H;
  bcfg.bit0.level1 = 0; bcfg.bit0.duration1 = T0L;
  bcfg.bit1.level0 = 1; bcfg.bit1.duration0 = T1H;
  bcfg.bit1.level1 = 0; bcfg.bit1.duration1 = T1L;
  bcfg.flags.msb_first = 1;
  if (rmt_new_bytes_encoder(&bcfg, &bytes_encoder_) != ESP_OK) return;

  rmt_copy_encoder_config_t cpy_cfg{};
  if (rmt_new_copy_encoder(&cpy_cfg, &copy_encoder_) != ESP_OK) return;

  if (rmt_enable(tx_channel_) != ESP_OK) return;

  tx_cfg_.loop_count = 0;
  rmt_ready_ = true;
}

// Writes
void ARGBStripComponent::update_group_channel(const std::string &group, uint8_t channel, uint8_t value) {
  std::string target_group = get_arm_select_group_name_();
  bool defer = (arm_select_mode_ != ArmSelectMode::NONE) &&
               (!target_group.empty()) &&
               (group == target_group) &&
               !arm_select_disable_pending_;

  if (defer) {
    auto &pend = pending_writes_[group];
    pend.used = true;
    if (channel <= 2) {
      pend.channel_set[channel] = true;
      pend.values[channel] = value;
    }
    return;
  }

  auto g = get_group(group);
  if (!g) return;
  for (int led : *g) {
    if (led < 0 || (uint16_t)led >= num_leds_) continue;
    uint32_t base = led * 3;
    switch (channel) {
      case 0: base_raw_grb_[base + 1] = value; break;
      case 1: base_raw_grb_[base + 0] = value; break;
      case 2: base_raw_grb_[base + 2] = value; break;
    }
  }
  if (!rfid_visual_active_()) mark_dirty_();
}

// Control
void ARGBStripComponent::enable_rfid_mode() {
  if (strip_mode_ == StripMode::RFID_PROGRAM && rfid_visual_active_()) return;
  strip_mode_ = StripMode::RFID_PROGRAM;
  rainbow_start_ms_ = millis();
  rfid_transition_ = RfidTransitionState::FADE_IN;
  rfid_transition_start_ms_ = rainbow_start_ms_;
  mark_dirty_();
}

void ARGBStripComponent::disable_rfid_mode() {
  if (strip_mode_ != StripMode::RFID_PROGRAM || rfid_transition_ == RfidTransitionState::FADE_OUT) return;
  rfid_transition_ = RfidTransitionState::FADE_OUT;
  rfid_transition_start_ms_ = millis();
  mark_dirty_();
}

void ARGBStripComponent::finish_rfid_fade_out_() {
  strip_mode_ = StripMode::NORMAL;
  rfid_transition_ = RfidTransitionState::INACTIVE;
  if (arm_select_disable_pending_) {
    finalize_arm_select_disable_();
  } else {
    mark_dirty_();
  }
}

void ARGBStripComponent::set_arm_select_mode(ArmSelectMode m) {
  if (arm_select_mode_ == m) return;

  std::string prev_group = get_arm_select_group_name_();
  std::string new_group;
  switch (m) {
    case ArmSelectMode::AWAY: new_group = "away"; break;
    case ArmSelectMode::HOME: new_group = "home"; break;
    case ArmSelectMode::DISARM: new_group = "disarm"; break;
    case ArmSelectMode::NIGHT:
    case ArmSelectMode::VACATION:
    case ArmSelectMode::BYPASS:
    case ArmSelectMode::ACTION: new_group = "custom"; break;
    default: break;
  }

  if (m == ArmSelectMode::NONE) {
    if (arm_select_mode_ != ArmSelectMode::NONE) {
      arm_select_disable_pending_ = true;
    }
    return;
  }

  if (arm_select_disable_pending_) {
    arm_select_disable_pending_ = false;
    if (!prev_group.empty()) apply_pending_for_group_(prev_group);
  }

  if (arm_select_mode_ != ArmSelectMode::NONE && !prev_group.empty() && prev_group != new_group) {
    apply_pending_for_group_(prev_group);
  }

  arm_select_mode_ = m;
  if (m == ArmSelectMode::ACTION) {
    action_rainbow_start_ms_ = millis();
  }

  if (arm_select_mode_ != ArmSelectMode::NONE && !new_group.empty()) {
    if (prev_group != new_group) {
      auto &pend = pending_writes_[new_group];
      pend.used = true;
      pend.channel_set = {false,false,false};
    }
  }

  if (!rfid_visual_active_()) mark_dirty_();
}

void ARGBStripComponent::finalize_arm_select_disable_() {
  std::string prev_group = get_arm_select_group_name_();
  if (!prev_group.empty()) apply_pending_for_group_(prev_group);
  arm_select_mode_ = ArmSelectMode::NONE;
  arm_select_disable_pending_ = false;
  if (!rfid_visual_active_()) mark_dirty_();
}

void ARGBStripComponent::set_arm_select_mode_by_name(const char *name) {
  if (!name) return;
  std::string s{name};
  std::transform(s.begin(), s.end(), s.begin(), ::tolower);
  ArmSelectMode m = ArmSelectMode::NONE;
  if (s == "away") m = ArmSelectMode::AWAY;
  else if (s == "home") m = ArmSelectMode::HOME;
  else if (s == "disarm") m = ArmSelectMode::DISARM;
  else if (s == "night") m = ArmSelectMode::NIGHT;
  else if (s == "vacation") m = ArmSelectMode::VACATION;
  else if (s == "bypass") m = ArmSelectMode::BYPASS;
  else if (s == "action") m = ArmSelectMode::ACTION;
  set_arm_select_mode(m);
}

void ARGBStripComponent::apply_pending_for_group_(const std::string &group) {
  auto it = pending_writes_.find(group);
  if (it == pending_writes_.end()) return;
  auto &pend = it->second;
  if (!pend.used) return;
  auto g = get_group(group);
  if (!g) return;

  for (int led : *g) {
    if (led < 0 || (uint16_t)led >= num_leds_) continue;
    uint32_t base = led * 3;
    if (pend.channel_set[0]) base_raw_grb_[base + 1] = pend.values[0];
    if (pend.channel_set[1]) base_raw_grb_[base + 0] = pend.values[1];
    if (pend.channel_set[2]) base_raw_grb_[base + 2] = pend.values[2];
  }
  pend.used = false;
  pend.channel_set = {false,false,false};
  if (!rfid_visual_active_()) mark_dirty_();
}

// Helpers
std::string ARGBStripComponent::get_arm_select_group_name_() const {
  switch (arm_select_mode_) {
    case ArmSelectMode::AWAY: return "away";
    case ArmSelectMode::HOME: return "home";
    case ArmSelectMode::DISARM: return "disarm";
    case ArmSelectMode::NIGHT:
    case ArmSelectMode::VACATION:
    case ArmSelectMode::BYPASS:
    case ArmSelectMode::ACTION: return "custom";
    default: return "";
  }
}

int ARGBStripComponent::get_arm_select_led_index_() const {
  std::string gname = get_arm_select_group_name_();
  if (gname.empty()) return -1;
  auto g = get_group(gname);
  if (!g || g->empty()) return -1;
  return (*g)[0];
}

bool ARGBStripComponent::rfid_visual_active_() const {
  if (strip_mode_ != StripMode::RFID_PROGRAM) return false;
  return (rfid_transition_ == RfidTransitionState::FADE_IN ||
          rfid_transition_ == RfidTransitionState::ACTIVE ||
          rfid_transition_ == RfidTransitionState::FADE_OUT);
}

float ARGBStripComponent::current_rfid_fade_factor_() const {
  if (!rfid_visual_active_()) return 0.0f;
  uint32_t now = millis();
  uint32_t elapsed = now - rfid_transition_start_ms_;
  if (rfid_transition_ == RfidTransitionState::FADE_IN) {
    if (elapsed >= RFID_FADE_MS) return 1.0f;
    return (float)elapsed / (float)RFID_FADE_MS;
  } else if (rfid_transition_ == RfidTransitionState::ACTIVE) {
    return 1.0f;
  } else if (rfid_transition_ == RfidTransitionState::FADE_OUT) {
    if (elapsed >= RFID_FADE_MS) return 0.0f;
    return 1.0f - (float)elapsed / (float)RFID_FADE_MS;
  }
  return 0.0f;
}

// Recomposition
void ARGBStripComponent::recomposite_() {
  if (rfid_visual_active_()) {
    float f = current_rfid_fade_factor_();
    build_rainbow_frame_(f);
  } else {
    build_normal_base_();
    if (arm_select_mode_ != ArmSelectMode::NONE || arm_select_disable_pending_) {
      apply_arm_select_overlay_();
    }
  }
  apply_group_caps_();
}

void ARGBStripComponent::build_normal_base_() {
  working_grb_ = base_raw_grb_;
}

void ARGBStripComponent::build_rainbow_frame_(float fade_factor) {
  if (num_leds_ == 0) return;
  working_grb_.assign(num_leds_ * 3, 0);
  if (fade_factor <= 0.0f) return;
  uint32_t now = millis();
  uint32_t elapsed = now - rainbow_start_ms_;
  float base_h = fmodf((float)elapsed / (float)rainbow_cycle_ms_, 1.0f);
  for (uint16_t i = 0; i < num_leds_; i++) {
    float h = fmodf(base_h + (float)i / (float)num_leds_, 1.0f);
    uint8_t g,r,b;
    hsv_to_grb_(h, 1.0f, 1.0f, g, r, b);
    if (fade_factor < 0.999f) {
      g = (uint8_t)(g * fade_factor + 0.5f);
      r = (uint8_t)(r * fade_factor + 0.5f);
      b = (uint8_t)(b * fade_factor + 0.5f);
    }
    uint32_t idx = i * 3;
    working_grb_[idx + 0] = g;
    working_grb_[idx + 1] = r;
    working_grb_[idx + 2] = b;
  }
}

void ARGBStripComponent::apply_arm_select_overlay_() {
  if (arm_select_mode_ == ArmSelectMode::NONE && !arm_select_disable_pending_)
    return;
  if (rfid_visual_active_()) return;

  // ACTION rainbow mode
  if (arm_select_mode_ == ArmSelectMode::ACTION) {
    const std::string group_name = "custom";
    const auto *grp = get_group(group_name);
    if (!grp || grp->empty()) return;
    uint32_t now = millis();
    float base = (float)((now - action_rainbow_start_ms_) % ACTION_RAINBOW_CYCLE_MS) / (float)ACTION_RAINBOW_CYCLE_MS;
    for (size_t idx = 0; idx < grp->size(); ++idx) {
      int led = (*grp)[idx];
      if (led < 0 || (uint16_t)led >= num_leds_) continue;
      float h = fmodf(base + (float)idx / (float)grp->size(), 1.0f);
      uint8_t g,r,b;
      hsv_to_grb_(h, 1.0f, 1.0f, g, r, b);
      uint32_t base_i = led * 3;
      working_grb_[base_i + 0] = g;
      working_grb_[base_i + 1] = r;
      working_grb_[base_i + 2] = b;
    }
    return;
  }

  // Existing flash logic
  int led_index = get_arm_select_led_index_();
  if (led_index < 0) return;
  uint32_t now = millis();
  uint32_t phase = (now % (FLASH_ON_MS + FLASH_OFF_MS));
  bool on_phase = phase < FLASH_ON_MS;

  uint8_t flash_r = 255, flash_g = 0, flash_b = 0;
  switch (arm_select_mode_) {
    case ArmSelectMode::VACATION: flash_r = 128; flash_g = 0; flash_b = 118; break;
    case ArmSelectMode::BYPASS:   flash_r = 255; flash_g = 80; flash_b = 0; break;
    default: break;
  }

  uint32_t base = led_index * 3;
  if (on_phase) {
    working_grb_[base + 0] = flash_g;
    working_grb_[base + 1] = flash_r;
    working_grb_[base + 2] = flash_b;
  } else {
    working_grb_[base + 0] = 0;
    working_grb_[base + 1] = 0;
    working_grb_[base + 2] = 0;
  }
}

// Scaling
void ARGBStripComponent::build_gamma_lut_if_needed_() {
  if (!build_gamma_lut_) return;
  build_gamma_lut_ = false;
  for (int i=0;i<256;i++) {
    float norm = i / 255.0f;
    gamma_lut_[i] = (uint8_t)(powf(norm, perceptual_gamma_) * 255.0f + 0.5f);
  }
}

void ARGBStripComponent::apply_group_caps_() {
  build_gamma_lut_if_needed_();
  for (auto &kv : groups_) {
    uint16_t cap = get_group_cap(kv.first);
    if (cap == 0) {
      // All off
      for (int led : kv.second) {
        if (led < 0 || (uint16_t)led >= num_leds_) continue;
        uint32_t base = led * 3;
        working_grb_[base+0]=0; working_grb_[base+1]=0; working_grb_[base+2]=0;
      }
      continue;
    }
    if (cap >= 255 && scaling_mode_ == ScalingMode::LINEAR) {
      // No change needed in pure linear full-bright case
      continue;
    }

    for (int led : kv.second) {
      if (led < 0 || (uint16_t)led >= num_leds_) continue;
      uint32_t base = led * 3;
      for (int c=0;c<3;c++) {
        uint8_t v = working_grb_[base + c];
        uint8_t out = v;
        switch (scaling_mode_) {
          case ScalingMode::LINEAR:
            out = (uint8_t)( (uint32_t)v * cap / 255 );
            break;
          case ScalingMode::CLAMP:
            if (v > cap) out = (uint8_t) cap;
            break;
          case ScalingMode::PERCEPTUAL: {
            // Apply perceptual curve then scale to cap
            uint8_t p = gamma_lut_[v];  // p in 0..255 perceptual
            out = (uint8_t)((uint32_t)p * cap / 255);
            break;
          }
        }
        working_grb_[base + c] = out;
      }
    }
  }
}

// Send
void ARGBStripComponent::send_frame_() {
  if (!rmt_ready_ || num_leds_ == 0) return;
  bool changed = false;
  for (size_t i=0;i<working_grb_.size(); i++) {
    if (working_grb_[i] != last_sent_grb_[i]) { changed = true; break; }
  }
  if (!changed) return;

  esp_err_t err = rmt_transmit(tx_channel_, bytes_encoder_, working_grb_.data(), working_grb_.size(), &tx_cfg_);
  if (err != ESP_OK) return;
  if (rmt_tx_wait_all_done(tx_channel_, pdMS_TO_TICKS(20)) != ESP_OK) return;
  err = rmt_transmit(tx_channel_, copy_encoder_, &reset_symbol_, sizeof(reset_symbol_), &tx_cfg_);
  if (err == ESP_OK) {
    rmt_tx_wait_all_done(tx_channel_, pdMS_TO_TICKS(10));
  }
  last_sent_grb_ = working_grb_;
}

// Color util
void ARGBStripComponent::hsv_to_grb_(float h, float s, float v, uint8_t &g, uint8_t &r, uint8_t &b) const {
  if (s <= 0.0f) {
    uint8_t val = (uint8_t)(v * 255.0f + 0.5f);
    g = r = b = val;
    return;
  }
  h = fmodf(h, 1.0f) * 6.0f;
  int i = (int) floorf(h);
  float f = h - i;
  float p = v * (1.0f - s);
  float q = v * (1.0f - s * f);
  float t = v * (1.0f - s * (1.0f - f));
  float rf,gf,bf;
  switch (i) {
    case 0: rf=v; gf=t; bf=p; break;
    case 1: rf=q; gf=v; bf=p; break;
    case 2: rf=p; gf=v; bf=t; break;
    case 3: rf=p; gf=q; bf=v; break;
    case 4: rf=t; gf=p; bf=v; break;
    default: rf=v; gf=p; bf=q; break;
  }
  r = (uint8_t)(rf * 255.0f + 0.5f);
  g = (uint8_t)(gf * 255.0f + 0.5f);
  b = (uint8_t)(bf * 255.0f + 0.5f);
}

// Output channel
void ARGBStripOutput::write_state(float state) {
  if (!parent_) return;
  if (state < 0.f) state = 0.f;
  if (state > 1.f) state = 1.f;
  uint8_t v = (uint8_t)(state * 255.0f + 0.5f);
  parent_->update_group_channel(group_, channel_, v);
}

}  // namespace argb_strip
}  // namespace esphome
#endif  // USE_ESP32
