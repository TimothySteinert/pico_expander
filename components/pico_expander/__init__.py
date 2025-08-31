import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.const import CONF_ID

CONF_PICO_EXPANDER = "pico_expander"

DEPENDENCIES = ["i2c"]
MULTI_CONF = True

pico_expander_ns = cg.esphome_ns.namespace("pico_expander")

PicoExpanderComponent = pico_expander_ns.class_(
    "PicoExpanderComponent", cg.Component, i2c.I2CDevice
)

CONFIG_SCHEMA = (
    cv.Schema({cv.Required(CONF_ID): cv.declare_id(PicoExpanderComponent)})
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x08))
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
