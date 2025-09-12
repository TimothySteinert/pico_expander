#include "k1_arm_handler.h"
#include "esphome/core/log.h"
#include "esphome/components/api/api_server.h"
#include "esphome/components/api/api_connection.h"

namespace esphome {
namespace k1_arm_handler {

static const char *const TAG = "k1_arm_handler";

// ------------------ Mapping Helpers ------------------
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

// ------------------ Frame Parser ------------------
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
  if (bytes[4] != 0xFF) {
    arm_select = map_arm_select_(bytes[4]);
  }

  std::string pin;
  for (int i = 5; i < 21; i++) {
    if (bytes[i] != 0xFF) {
      pin += map_digit_(bytes[i]);
    }
  }

  if (arm_select.empty() || arm_select == "unknown" || pin.empty()) {
    ESP_LOGW(TAG, "Invalid A0: arm_select='%s' pin_len=%u",
             arm_select.c_str(), (unsigned) pin.size());
    return;
  }

  ESP_LOGD(TAG, "Parsed A0 -> prefix='%s' arm_select='%s' pin='%s'",
           prefix.c_str(), arm_select.c_str(), pin.c_str());

  this->execute_command(prefix, arm_select, pin);
}

// ------------------ Public Execute ------------------
void K1ArmHandlerComponent::execute_command(const std::string &prefix,
                                            const std::string &arm_select,
                                            const std::string &pin) {
  if (alarm_entity_id_.empty()) {
    ESP_LOGE(TAG, "Alarm entity ID not set");
    return;
  }

  bool force_flag = (prefix == force_prefix_);
  bool skip_delay_flag = (prefix == skip_delay_prefix_);

  // If you want to mask the pin in logs:
  // std::string pin_for_log(pin.size() ? "****" : "");
  const std::string &pin_for_log = pin;

  ESP_LOGI(TAG,
           "Execute - mode=%s pin='%s' force=%s skip_delay=%s (prefix='%s')",
           arm_select.c_str(), pin_for_log.c_str(),
           force_flag ? "true" : "false",
           skip_delay_flag ? "true" : "false",
           prefix.c_str());

  if (arm_select == "disarm") {
    call_disarm_(pin);
  } else {
    call_arm_(arm_select, pin, force_flag, skip_delay_flag);
  }
}

// ------------------ Service Helpers ------------------
void K1ArmHandlerComponent::call_disarm_(const std::string &pin) {
  if (disarm_service_.empty()) {
    ESP_LOGE(TAG, "Disarm service string empty");
    return;
  }
  if (api::global_api_server == nullptr) {
    ESP_LOGW(TAG, "API server not ready (disarm) - will not send");
    return;
  }

  ESP_LOGI(TAG, "HA service disarm: %s entity=%s",
           disarm_service_.c_str(), alarm_entity_id_.c_str());

  api::HomeAssistantServiceCallRequest req;
  req.service = disarm_service_;
  req.data["entity_id"] = alarm_entity_id_;
  req.data["code"] = pin;

  api::global_api_server->send_homeassistant_service_call(req);
}

void K1ArmHandlerComponent::call_arm_(const std::string &mode,
                                      const std::string &pin,
                                      bool force_flag,
                                      bool skip_delay_flag) {
  if (arm_service_.empty()) {
    ESP_LOGE(TAG, "Arm service string empty");
    return;
  }
  if (api::global_api_server == nullptr) {
    ESP_LOGW(TAG, "API server not ready (arm) - will not send");
    return;
  }

  ESP_LOGI(TAG, "HA service arm: %s entity=%s mode=%s force=%s skip_delay=%s",
           arm_service_.c_str(), alarm_entity_id_.c_str(),
           mode.c_str(),
           force_flag ? "true" : "false",
           skip_delay_flag ? "true" : "false");

  api::HomeAssistantServiceCallRequest req;
  req.service = arm_service_;
  req.data["entity_id"]  = alarm_entity_id_;
  req.data["mode"]       = mode;
  req.data["code"]       = pin;
  // Alarmo expects booleans; HA service API will coerce strings "true"/"false" or actual bool.
  req.data["force"]      = force_flag ? "true" : "false";
  req.data["skip_delay"] = skip_delay_flag ? "true" : "false";

  api::global_api_server->send_homeassistant_service_call(req);
}

}  // namespace k1_arm_handler
}  // namespace esphhome
