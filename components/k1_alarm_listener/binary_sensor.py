import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_ID

from . import K1AlarmListener

CONF_K1_ALARM_LISTENER_ID = "k1_alarm_listener_id"
CONF_TYPE = "type"

# Aliases all map to the same enum (0)
BINARY_SENSOR_TYPES = {
    "ha_connected": 0,
    "ha_available": 0,
    "api_connected": 0,
    "connection": 0,
}

K1AlarmListenerBinarySensor = cg.esphome_ns.namespace("k1_alarm_listener").class_(
    "K1AlarmListenerBinarySensor",
    binary_sensor.BinarySensor,
    cg.Component
)

CONFIG_SCHEMA = binary_sensor.BINARY_SENSOR_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(K1AlarmListenerBinarySensor),
        cv.GenerateID(CONF_K1_ALARM_LISTENER_ID): cv.use_id(K1AlarmListener),
        cv.Optional(CONF_TYPE, default="ha_connected"): cv.one_of(*BINARY_SENSOR_TYPES.keys(), lower=True),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_K1_ALARM_LISTENER_ID])
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await binary_sensor.register_binary_sensor(var, config)
    cg.add(var.set_parent(parent))
    cg.add(var.set_type(BINARY_SENSOR_TYPES[config[CONF_TYPE]]))
