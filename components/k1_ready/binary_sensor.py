import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_ID

from . import k1_ready_ns, K1Ready

K1ReadyBinarySensor = k1_ready_ns.class_("K1ReadyBinarySensor", binary_sensor.BinarySensor, cg.Component)

CONF_K1_READY_ID = "k1_ready_id"

CONFIG_SCHEMA = binary_sensor.binary_sensor_schema(K1ReadyBinarySensor).extend(
    {
        cv.GenerateID(): cv.declare_id(K1ReadyBinarySensor),
        cv.Required(CONF_K1_READY_ID): cv.use_id(K1Ready),
    }
)

async def to_code(config):
    engine = await cg.get_variable(config[CONF_K1_READY_ID])
    var = cg.new_Pvariable(config[CONF_ID], engine)
    await cg.register_component(var, config)
    await binary_sensor.register_binary_sensor(var, config)
    cg.add(engine.register_ready_binary_sensor(var))
