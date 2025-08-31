import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import i2c
from esphome.const import (
    CONF_ID,
    CONF_NUMBER,
    CONF_MODE,
    CONF_INVERTED,
    CONF_INPUT,
    CONF_OUTPUT,
)

CONF_PICO_EXPANDER = "pico_expander"

DEPENDENCIES = ["i2c"]
MULTI_CONF = True

pico_expander_ns = cg.esphome_ns.namespace("pico_expander")

PicoExpanderComponent = pico_expander_ns.class_(
    "PicoExpanderComponent", cg.Component, i2c.I2CDevice
)
PicoExpanderGPIOPin = pico_expander_ns.class_(
    "PicoExpanderGPIOPin", cg.GPIOPin
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.Required(CONF_ID): cv.declare_id(PicoExpanderComponent),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x08))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)


def validate_mode(value):
    if not (value[CONF_INPUT] or value[CONF_OUTPUT]):
        raise cv.Invalid("Mode must be either input or output")
    if value[CONF_INPUT] and value[CONF_OUTPUT]:
        raise cv.Invalid("Mode must be either input or output")
    return value


PICO_EXPANDER_PIN_SCHEMA = cv.All(
    {
        cv.GenerateID(): cv.declare_id(PicoExpanderGPIOPin),
        cv.Required(CONF_PICO_EXPANDER): cv.use_id(PicoExpanderComponent),
        cv.Required(CONF_NUMBER): cv.All(
            cv.int_range(min=0, max=1),  # only GPIO 0 and 1
        ),
        cv.Optional(CONF_MODE, default={}): cv.All(
            {
                cv.Optional(CONF_INPUT, default=False): cv.boolean,
                cv.Optional(CONF_OUTPUT, default=False): cv.boolean,
            },
            validate_mode,
        ),
        cv.Optional(CONF_INVERTED, default=False): cv.boolean,
    }
)


@pins.PIN_SCHEMA_REGISTRY.register(
    CONF_PICO_EXPANDER, PICO_EXPANDER_PIN_SCHEMA
)
async def PicoExpander_pin_to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    parent = await cg.get_variable(config[CONF_PICO_EXPANDER])

    cg.add(var.set_parent(parent))

    num = config[CONF_NUMBER]
    cg.add(var.set_pin(num))
    cg.add(var.set_inverted(config[CONF_INVERTED]))
    cg.add(var.set_flags(pins.gpio_flags_expr(config[CONF_MODE])))
    return var
