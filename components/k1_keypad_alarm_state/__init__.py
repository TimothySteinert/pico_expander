import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ENTITY_ID, CONF_ID

k1_ns = cg.esphome_ns.namespace("k1_keypad_alarm_state")
K1KeypadAlarmState = k1_ns.class_("K1KeypadAlarmState", text_sensor.TextSensor, cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(K1KeypadAlarmState),
    cv.Required(CONF_ENTITY_ID): cv.entity_id,
}).extend(text_sensor.text_sensor_schema(K1KeypadAlarmState))

async def to_code(config):
    var = await text_sensor.new_text_sensor(config)
    cg.add(var.set_entity_id(config[CONF_ENTITY_ID]))
