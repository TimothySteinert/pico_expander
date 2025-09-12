import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_ID

from . import k1_alarm_listener_ns, K1AlarmListener

CONF_K1_ALARM_LISTENER_ID = "k1_alarm_listener_id"
CONF_TYPE = "type"

# Only one type right now, keep extensible
TYPE_MAP = {
    "ha_connected": 0,
}

K1AlarmListenerBinarySensor = k1_alarm_listener_ns.class_(
    "K1AlarmListenerBinarySensor", binary_sensor.BinarySensor, cg.Component
)

CONFIG_SCHEMA = (
    binary_sensor.binary_sensor_schema(K1AlarmListenerBinarySensor)
    .extend(
        {
            cv.GenerateID(CONF_K1_ALARM_LISTENER_ID): cv.use_id(K1AlarmListener),
            cv.Optional(CONF_TYPE, default="ha_connected"): cv.one_of(
                *TYPE_MAP.keys(), lower=True
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)

async def to_code(config):
    parent = await cg.get_variable(config[CONF_K1_ALARM_LISTENER_ID])
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await binary_sensor.register_binary_sensor(var, config)
    cg.add(var.set_parent(parent))
    cg.add(var.set_type(TYPE_MAP[config[CONF_TYPE]]))
