import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID

from . import K1AlarmListener

CONF_K1_ALARM_LISTENER_ID = "k1_alarm_listener_id"

K1AlarmListenerTextSensor = cg.esphome_ns.namespace("k1_alarm_listener").class_(
    "K1AlarmListenerTextSensor",
    text_sensor.TextSensor,
    cg.Component
)

CONFIG_SCHEMA = text_sensor.TEXT_SENSOR_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(K1AlarmListenerTextSensor),
        cv.GenerateID(CONF_K1_ALARM_LISTENER_ID): cv.use_id(K1AlarmListener),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_K1_ALARM_LISTENER_ID])
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await text_sensor.register_text_sensor(var, config)
    cg.add(var.set_parent(parent))
