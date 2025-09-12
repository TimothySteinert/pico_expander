import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID

from . import k1_alarm_listener_ns, K1AlarmListener

CONF_K1_ALARM_LISTENER_ID = "k1_alarm_listener_id"

# C++ class (already declared in k1_alarm_listener.h/cpp)
K1AlarmListenerTextSensor = k1_alarm_listener_ns.class_(
    "K1AlarmListenerTextSensor", text_sensor.TextSensor, cg.Component
)

CONFIG_SCHEMA = (
    text_sensor.text_sensor_schema(K1AlarmListenerTextSensor)
    .extend(
        {
            cv.GenerateID(CONF_K1_ALARM_LISTENER_ID): cv.use_id(K1AlarmListener),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)

async def to_code(config):
    parent = await cg.get_variable(config[CONF_K1_ALARM_LISTENER_ID])
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await text_sensor.register_text_sensor(var, config)
    cg.add(var.set_parent(parent))
