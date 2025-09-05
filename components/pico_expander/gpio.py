import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.const import CONF_ID, CONF_NUMBER

from . import PicoExpanderComponent, CONF_PICO_EXPANDER

pico_expander_ns = cg.esphome_ns.namespace("pico_expander")
PicoExpanderGPIOPin = pico_expander_ns.class_("PicoExpanderGPIOPin", cg.GPIOPin)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(PicoExpanderGPIOPin),
        cv.Required(CONF_PICO_EXPANDER): cv.use_id(PicoExpanderComponent),
        cv.Required(CONF_NUMBER): cv.int_range(min=0x40, max=0x4F),
    }
)

@pins.PIN_SCHEMA_REGISTRY.register(CONF_PICO_EXPANDER, CONFIG_SCHEMA)
async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    parent = await cg.get_variable(config[CONF_PICO_EXPANDER])
    cg.add(var.set_parent(parent))
    cg.add(var.set_channel(config[CONF_NUMBER]))
    return var
