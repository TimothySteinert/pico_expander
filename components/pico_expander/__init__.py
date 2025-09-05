import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import i2c
from esphome.const import CONF_ID, CONF_NUMBER

CONF_PICO_EXPANDER = "pico_expander"

DEPENDENCIES = ["i2c"]
MULTI_CONF = True

pico_expander_ns = cg.esphome_ns.namespace("pico_expander")

PicoExpanderComponent = pico_expander_ns.class_(
    "PicoExpanderComponent", cg.Component, i2c.I2CDevice
)

PicoExpanderGPIOPin = pico_expander_ns.class_("PicoExpanderGPIOPin", cg.GPIOPin)

# ----- Hub schema -----
CONFIG_SCHEMA = (
    cv.Schema({cv.Required(CONF_ID): cv.declare_id(PicoExpanderComponent)})
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x08))
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

# ----- GPIO pin schema -----
PICO_EXPANDER_PIN_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(PicoExpanderGPIOPin),
        cv.Required(CONF_PICO_EXPANDER): cv.use_id(PicoExpanderComponent),
        cv.Required(CONF_NUMBER): cv.int_range(min=0x40, max=0x4F),
        cv.Optional("mode", default={"output": True}): pins.gpio_flags,  # ✅ match Arduino
        cv.Optional("inverted", default=False): cv.boolean,              # optional
    }
)

@pins.PIN_SCHEMA_REGISTRY.register(CONF_PICO_EXPANDER, PICO_EXPANDER_PIN_SCHEMA)
async def pico_expander_pin_to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    parent = await cg.get_variable(config[CONF_PICO_EXPANDER])
    cg.add(var.set_parent(parent))
    cg.add(var.set_channel(config[CONF_NUMBER]))
    cg.add(var.set_flags(pins.gpio_flags_expr(config["mode"])))
    cg.add(var.set_inverted(config["inverted"]))
    return var

# ----- Import outputs so LED support is active -----
from . import output
