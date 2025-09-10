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
namespace esphome {
namespace argb_strip {

static const char *const TAG = "argb_strip";

static const uint32_t T0H = 16;
static const uint32_t T0L = 34;
static const uint32_t T1H = 32;
static const uint32_t T1L = 18;

// ---------------- Group Management ----------------
void ARGBStripComponent::add_group(const std::string &name, const std::vector<int> &leds, float max_brightness) {
  groups_[name] = leds;
  group_max_[name] = max_brightness;
}
const std::vector<int> *ARGBStripComponent::get_group(const std::string &name) const {
  auto it = groups_.find(name);
  if (it == groups_.end()) return nullptr;
  return &it->second;
}
float ARGBStripComponent::get_group_max(const std::string &name) const {
  auto it = group_max_.find(name);
  if (it == groups_.end()) return 1.0f;
  return it->second;
}
uint8_t ARGBStripComponent::scale_group_value_(const std::string &group, uint8_t v) const {
  float gm = get_group_max(group);
  float scaled = v * gm;
  if (scaled > 255.f) scaled = 255.f;
  return (uint8_t)(scaled + 0.5f);
}

// ---------------- Lifecycle ----------------
void ARGBStripComponent::setup() {
  ESP_LOGCONFIG(TAG, "ARGB Strip v%s (flash off-phase + graceful disable)", ARGB_STRIP_VERSION);
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
  mark_dirty_();
}

void ARGBStripComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "ARGB Strip (internal control):");
  LOG_PIN("  Pin: ", pin_);
  ESP_LOGCONFIG(TAG, "  Raw GPIO: %d", raw_gpio_);
  ESP_LOGCONFIG(TAG, "  LEDs: %u", num_leds_);
  ESP_LOGCONFIG(TAG, "  RMT Ready: %s", rmt_ready_ ? "YES" : "NO");
  ESP_LOGCONFIG(TAG, "  Strip Mode: %s", strip_mode_ == StripMode::NORMAL ? "NORMAL" : "RFID_PROGRAM");
  ESP_LOGCONFIG(TAG, "  RFID Transition State: %d", (int) rfid_transition_);
  ESP_LOGCONFIG(TAG, "  Arm Select Mode: %d", (int) arm_select_mode_);
  ESP_LOGCONFIG(TAG, "  Arm Select Disable Pending: %s", arm_select_disable_pending_ ? "YES" : "NO");
  if (!groups_.empty()) {
    ESP_LOGCONFIG(TAG, "  Groups:");
    for (auto &kv : groups_) {
      ESP_LOGCONFIG(TAG, "    %s -> %u pixel(s), max=%.3f",
                    kv.first.c_str(), (unsigned) kv.second.size(), get_group_max(kv.first));
    }
  }
}

// ---------------- Loop ----------------
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
      mark_dirty_();
    }
    // Check if we should finalize a pending disable (only when not in RFID visual)
    if (arm_select_disable_pending_) {
      // Determine phase
      uint32_t phase = (now % (FLASH_ON_MS + FLASH_OFF_MS));
      bool in_off_phase = phase >= FLASH_ON_MS; // OFF phase is the latter part
      if (in_off_phase) {
        finalize_arm_select_disable_();
      }
    }
  }

  if (anim_tick)
    last_anim_eval_ = now;

  if (frame_dirty_) {
    recomposite_();
    send_frame_();
    frame_dirty_ = false;
  }
}

// ---------------- RMT Init ----------------
void ARGBStripComponent::init_rmt_() {
  rmt_tx_channel_config_t ch_cfg{};
  ch_cfg.gpio_num = (gpio_num_t) raw_gpio_;
  ch_cfg.clk_src = RMT_CLK_SRC_DEFAULT;
  ch_cfg.resolution_hz = 40'000'000;
  ch_cfg.mem_block_symbols = 64;
  ch_cfg.trans_queue_depth = 4;

  if (rmt_new_tx_channel(&ch_cfg, &tx_channel_) != ESP_OK) {
    ESP_LOGE(TAG, "rmt_new_tx_channel failed");
    return;
  }

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

// ---------------- Base Channel Writes ----------------
void ARGBStripComponent::update_group_channel(const std::string &group, uint8_t channel, uint8_t value) {
  std::string target_group = get_arm_select_group_name_();
  bool defer = (arm_select_mode_ != ArmSelectMode::NONE) &&
               (!target_group.empty()) &&
               (group == target_group) &&
               !arm_select_disable_pending_;
  // If disable is pending we still defer until finalization.

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
    if (led < 0 || (uint16_t) led >= num_leds_) continue;
    uint32_t base = led * 3;
    switch (channel) {
      case 0: base_raw_grb_[base + 1] = value; break; // R
      case 1: base_raw_grb_[base + 0] = value; break; // G
      case 2: base_raw_grb_[base + 2] = value; break; // B
    }
  }
  if (!rfid_visual_active_()) {
    mark_dirty_();
  }
}

// ---------------- Internal Control ----------------
void ARGBStripComponent::enable_rfid_mode() {
  if (strip_mode_ == StripMode::RFID_PROGRAM && rfid_visual_active_()) return;
  strip_mode_ = StripMode::RFID_PROGRAM;
  rainbow_start_ms_ = millis();
  rfid_transition_ = RfidTransitionState::FADE_IN;
  rfid_transition_start_ms_ = rainbow_start_ms_;
  mark_dirty_();
}

void ARGBStripComponent::disable_rfid_mode() {
  if (strip_mode_ != StripMode::RFID_PROGRAM || rfid_transition_ == RfidTransitionState::FADE_OUT)
    return;
  rfid_transition_ = RfidTransitionState::FADE_OUT;
  rfid_transition_start_ms_ = millis();
  mark_dirty_();
}

void ARGBStripComponent::finish_rfid_fade_out_() {
  strip_mode_ = StripMode::NORMAL;
  rfid_transition_ = RfidTransitionState::INACTIVE;
  // If we had a pending arm-select disable and RFID suppressed the flash,
  // finalize immediately (no need to wait for OFF phase because flash not visible).
  if (arm_select_disable_pending_) {
    finalize_arm_select_disable_();
  } else {
    mark_dirty_();
  }
}

// Modified: supports pending disable logic
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
      new_group = "custom"; break;
    default: break; // NONE
  }

  // If we are changing to NONE, initiate pending disable logic
  if (m == ArmSelectMode::NONE) {
    if (arm_select_mode_ != ArmSelectMode::NONE) {
      // Start pending disable (do not flush yet)
      arm_select_disable_pending_ = true;
      // We keep current mode active until OFF phase is reached
      // (rfid_visual_active_() suppresses overlay anyway)
    }
    return; // Do not change arm_select_mode_ yet
  }

  // If there was a pending disable and user selects a new mode instead: cancel pending disable
  if (arm_select_disable_pending_) {
    arm_select_disable_pending_ = false;
    // Flush previous group's pending writes (since we are leaving that selection path)
    if (!prev_group.empty())
      apply_pending_for_group_(prev_group);
  }

  // Normal cross-mode switch behavior:
  if (arm_select_mode_ != ArmSelectMode::NONE && !prev_group.empty() && prev_group != new_group) {
    apply_pending_for_group_(prev_group);
  }

  arm_select_mode_ = m;

  if (arm_select_mode_ != ArmSelectMode::NONE && !new_group.empty()) {
    if (prev_group != new_group) {
      auto &pend = pending_writes_[new_group];
      pend.used = true;
      pend.channel_set = {false,false,false};
    }
  }

  if (!rfid_visual_active_()) {
    mark_dirty_();
  }
}

void ARGBStripComponent::finalize_arm_select_disable_() {
  // We are currently still in a mode (not NONE yet), apply pending writes
  std::string prev_group = get_arm_select_group_name_();
  if (!prev_group.empty())
    apply_pending_for_group_(prev_group);

  arm_select_mode_ = ArmSelectMode::NONE;
  arm_select_disable_pending_ = false;
  if (!rfid_visual_active_()) {
    mark_dirty_();
  }
}

void ARGBStripComponent::set_arm_select_mode_by_name(const char *name) {
  if (!name) return;
  std::string s{name};
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return (char) std::tolower(c); });
  ArmSelectMode m = ArmSelectMode::NONE;
  if (s == "away") m = ArmSelectMode::AWAY;
  else if (s == "home") m = ArmSelectMode::HOME;
  else if (s == "disarm") m = ArmSelectMode::DISARM;
  else if (s == "night") m = ArmSelectMode::NIGHT;
  else if (s == "vacation") m = ArmSelectMode::VACATION;
  else if (s == "bypass") m = ArmSelectMode::BYPASS;
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
    if (led < 0 || (uint16_t) led >= num_leds_) continue;
    uint32_t base = led * 3;
    if (pend.channel_set[0]) base_raw_grb_[base + 1] = pend.values[0];
    if (pend.channel_set[1]) base_raw_grb_[base + 0] = pend.values[1];
    if (pend.channel_set[2]) base_raw_grb_[base + 2] = pend.values[2];
  }
  pend.used = false;
  pend.channel_set = {false,false,false};
  if (!rfid_visual_active_()) {
    mark_dirty_();
  }
}

// ---------------- Helpers ----------------
std::string ARGBStripComponent::get_arm_select_group_name_() const {
  switch (arm_select_mode_) {
    case ArmSelectMode::AWAY: return "away";
    case ArmSelectMode::HOME: return "home";
    case ArmSelectMode::DISARM: return "disarm";
    case ArmSelectMode::NIGHT:
    case ArmSelectMode::VACATION:
    case ArmSelectMode::BYPASS:
      return "custom";
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
    return (float) elapsed / (float) RFID_FADE_MS;
  } else if (rfid_transition_ == RfidTransitionState::ACTIVE) {
    return 1.0f;
  } else if (rfid_transition_ == RfidTransitionState::FADE_OUT) {
    if (elapsed >= RFID_FADE_MS) return 0.0f;
    return 1.0f - (float) elapsed / (float) RFID_FADE_MS;
  }
  return 0.0f;
}

// ---------------- Recomposition ----------------
void ARGBStripComponent::recomposite_() {
  if (rfid_visual_active_()) {
    float f = current_rfid_fade_factor_();
    build_rainbow_frame_(f);
  } else {
    build_normal_base_();
    if ((arm_select_mode_ != ArmSelectMode::NONE || arm_select_disable_pending_)) {
      apply_arm_select_overlay_();
    }
  }
}

void ARGBStripComponent::build_normal_base_() {
  working_grb_ = base_raw_grb_;
  for (auto &kv : groups_) {
    float gm = get_group_max(kv.first);
    if (gm >= 0.999f) continue;
    for (int led : kv.second) {
      if (led < 0 || (uint16_t) led >= num_leds_) continue;
      uint32_t b = led * 3;
      for (int i=0;i<3;i++) {
        float scaled = working_grb_[b+i] * gm;
        if (scaled > 255.f) scaled = 255.f;
        working_grb_[b+i] = (uint8_t)(scaled + 0.5f);
      }
    }
  }
}

void ARGBStripComponent::build_rainbow_frame_(float fade_factor) {
  if (num_leds_ == 0) return;
  working_grb_.assign(num_leds_ * 3, 0);
  if (fade_factor <= 0.0f) return;

  uint32_t now = millis();
  uint32_t elapsed = now - rainbow_start_ms_;
  float base_h = fmodf((float) elapsed / (float) RAINBOW_CYCLE_MS, 1.0f);

  for (uint16_t i = 0; i < num_leds_; i++) {
    float h = fmodf(base_h + (float) i / num_leds_, 1.0f);
    uint8_t g, r, b;
    hsv_to_grb_(h, 1.0f, 1.0f, g, r, b);
    if (fade_factor < 0.999f) {
      g = (uint8_t)(g * fade_factor + 0.5f);
      r = (uint8_t)(r * fade_factor + 0.5f);
      b = (uint8_t)(b * fade_factor + 0.5f);
    }
    uint32_t idx = i * 3;
    working_grb_[idx+0] = g;
    working_grb_[idx+1] = r;
    working_grb_[idx+2] = b;
  }
}

void ARGBStripComponent::apply_arm_select_overlay_() {
  // If fully disabled (no mode and not pending), nothing
  if (arm_select_mode_ == ArmSelectMode::NONE && !arm_select_disable_pending_)
    return;
  if (rfid_visual_active_()) return; // suppressed

  int led_index = get_arm_select_led_index_();
  if (led_index < 0) return;

  uint32_t now = millis();
  uint32_t phase = (now % (FLASH_ON_MS + FLASH_OFF_MS));
  bool on_phase = phase < FLASH_ON_MS;

  uint8_t flash_r = 255, flash_g = 0, flash_b = 0;
  switch (arm_select_mode_) {
    case ArmSelectMode::VACATION:
      flash_r = 128; flash_g = 0; flash_b = 118;
      break;
    case ArmSelectMode::BYPASS:
      flash_r = 255; flash_g = 80; flash_b = 0;
      break;
    default:
      break;
  }

  uint32_t base = led_index * 3;
  if (on_phase) {
    working_grb_[base + 0] = flash_g;
    working_grb_[base + 1] = flash_r;
    working_grb_[base + 2] = flash_b;
  } else {
    // Force OFF (0,0,0) regardless of underlying base color
    working_grb_[base + 0] = 0;
    working_grb_[base + 1] = 0;
    working_grb_[base + 2] = 0;
  }
}

// ---------------- Sending ----------------
void ARGBStripComponent::send_frame_() {
  if (!rmt_ready_ || num_leds_ == 0) return;
  bool changed = false;
  for (size_t i=0;i<working_grb_.size(); i++) {
    if (working_grb_[i] != last_sent_grb_[i]) { changed = true; break; }
  }
  if (!changed) return;

  esp_err_t err = rmt_transmit(tx_channel_, bytes_encoder_, working_grb_.data(), working_grb_.size(), &tx_cfg_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "rmt_transmit (data) failed err=0x%X", err);
    return;
  }
  err = rmt_tx_wait_all_done(tx_channel_, pdMS_TO_TICKS(20));
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "rmt_tx_wait_all_done (data) err=0x%X", err);
    return;
  }
  err = rmt_transmit(tx_channel_, copy_encoder_, &reset_symbol_, sizeof(reset_symbol_), &tx_cfg_);
  if (err == ESP_OK) {
    rmt_tx_wait_all_done(tx_channel_, pdMS_TO_TICKS(10));
  }
  last_sent_grb_ = working_grb_;
}

// ---------------- Color Utility ----------------
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
  float rf, gf, bf;
  switch (i) {
    case 0: rf = v; gf = t; bf = p; break;
    case 1: rf = q; gf = v; bf = p; break;
    case 2: rf = p; gf = v; bf = t; break;
    case 3: rf = p; gf = q; bf = v; break;
    case 4: rf = t; gf = p; bf = v; break;
    default: rf = v; gf = p; bf = q; break;
  }
  r = (uint8_t)(rf * 255.0f + 0.5f);
  g = (uint8_t)(gf * 255.0f + 0.5f);
  b = (uint8_t)(bf * 255.0f + 0.5f);
}

// ---------------- Output Channel ----------------
void ARGBStripOutput::write_state(float state) {
  if (!parent_) return;
  if (state < 0.f) state = 0.f;
  if (state > 1.f) state = 1.f;
  uint8_t v = (uint8_t)(state * 255.0f + 0.5f);
  parent_->update_group_channel(group_, channel_, v);
}

}  // namespace argb_strip
}  // namespace esphome
#endif
