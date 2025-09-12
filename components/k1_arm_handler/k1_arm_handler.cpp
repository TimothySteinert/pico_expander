#include "k1_arm_handler.h"
#include "esphome/core/log.h"

namespace esphome {
namespace k1_arm_handler {

static const char *const TAG = "k1_arm_handler";

// -------- Mapping Helpers --------
std::string K1ArmHandlerComponent::map_digit_(uint8_t code) const {
  switch (code) {
    case 0x05: return "1";
    case 0x0A: return "2";
    case 0x0F: return "3";
    case 0x11: return "4";
    case 0x16: return "5";
    case 0x1B: return "6";
    case 0x1C: return "7";
    case 0x22: return "8";
    case 0x27: return "9";
    case 0x00: return "0";
    default:   return "";
  }
}

const char *K1ArmHandlerComponent::map_arm_select_(uint8_t code) const {
  switch (code) {
    case 0x41: return "away";
    case 0x42: return "home";
    case 0x43: return "disarm";
    default:   return "unknown";
  }
}

// -------- Frame Parser --------
void K1ArmHandlerComponent::handle_a0_message(const std::vector<uint8_t> &bytes) {
  if (bytes.size() < 21) {
    ESP_LOGW(TAG, "A0 message too short (%u)", (unsigned) bytes.size());
    return;
  }
  if (bytes[0] != 0xA0) {
    ESP_LOGV(TAG, "Not an A0 frame (first=0x%02X)", bytes[0]);
    return;
  }

  std::string prefix;
  for (int i = 1; i <= 3; i++) {
    if (bytes[i] != 0xFF) {
      prefix += map_digit_(bytes[i]);
    }
  }

  std::string arm_select;
  if (bytes[4] != 0xFF)
    arm_select = map_arm_select_(bytes[4]);

  std::string pin;
  for (int i = 5; i < 21; i++) {
    if (bytes[i] != 0xFF) {
      pin += map_digit_(bytes[i]);
    }
  }

  if (arm_select.empty() || arm_select == "unknown" || pin.empty()) {
    ESP_LOGW(TAG, "Invalid A0 frame: arm_select='%s' pin_len=%u",
             arm_select.c_str(), (unsigned) pin.size());
    return;
  }

  ESP_LOGD(TAG, "Parsed A0 -> prefix='%s' arm_select='%s' pin='%s'",
           prefix.c_str(), arm_select.c_str(), pin.c_str());

  execute_command(prefix, arm_select, pin);
}

// -------- Execute Command --------
void K1ArmHandlerComponent::execute_command(const std::string &prefix,
                                            const std::string &arm_select,
                                            const std::string &pin) {
  bool force_flag = (prefix == force_prefix_);
  bool skip_delay_flag = (prefix == skip_delay_prefix_);

  ESP_LOGI(TAG,
           "Execute - mode=%s pin='%s' force=%s skip_delay=%s (prefix='%s')",
           arm_select.c_str(), pin.c_str(),
           force_flag ? "true" : "false",
           skip_delay_flag ? "true" : "false",
           prefix.c_str());

  invoke_script_(arm_select, pin, force_flag, skip_delay_flag, prefix);
}

// -------- Invoke Script --------
void K1ArmHandlerComponent::invoke_script_(const std::string &mode,
                                           const std::string &pin,
                                           bool force_flag,
                                           bool skip_delay_flag,
                                           const std::string &prefix) {
  if (!callback_script_) {
    ESP_LOGW(TAG, "No callback script configured; doing nothing.");
    return;
  }

  // Script parameters (must match YAML 'parameters:' order)
  std::vector<std::string> params;
  params.emplace_back(mode);                     // param 0
  params.emplace_back(pin);                      // param 1
  params.emplace_back(force_flag ? "1" : "0");   // param 2
  params.emplace_back(skip_delay_flag ? "1" : "0"); // param 3
  params.emplace_back(prefix);                   // param 4

  ESP_LOGD(TAG, "Calling script with params: mode=%s pin=%s force=%s skip=%s prefix=%s",
           params[0].c_str(), params[1].c_str(),
           params[2].c_str(), params[3].c_str(), params[4].c_str());

  callback_script_->execute(params);
}

}  // namespace k1_arm_handler
}  // namespace esphome
