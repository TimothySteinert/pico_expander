import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor, homeassistant
from esphome.const import CONF_ENTITY_ID, CONF_ID

DEPENDENCIES = ["homeassistant"]

k1_ns = cg.esphome_ns.namespace("k1_keypad_alarm_state")
K1KeypadAlarmState = k1_ns.class_("K1KeypadAlarmState", text_sensor.TextSensor, cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(K1KeypadAlarmState),
    cv.Required(CONF_ENTITY_ID): cv.entity_id,  # e.g. alarm_control_panel.alarmo
})


def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])

    # Create a Home Assistant text sensor to subscribe to the alarm entity
    ha_text = homeassistant.new_text_sensor(config[CONF_ENTITY_ID])

    # Whenever HA entity changes, map its state and publish
    def _callback(state):
        if state in ("unavailable", "unknown"):
            mapped = "connection_timed_out"
        else:
            mapped = state
        var.publish_state(mapped)

    ha_text.add_on_state_callback(_callback)

    cg.add(var)
