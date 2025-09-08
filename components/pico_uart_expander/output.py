import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import output
from esphome.const import CONF_ID, CONF_NUMBER

from . import pico_uart_expander_ns, PicoUARTExpanderComponent

PicoUARTExpanderOutput = pico_uart_expander_ns.class_(
    "PicoUARTExpanderOutput", output.FloatOutput
)

CONFIG_SCHEMA = output.FLOAT_OUTPUT_SCHEMA.extend(
    {
        cv.GenerateID(CONF_ID): cv.declare_id(PicoUARTExpanderOutput),
        cv.Required("pico_uart_expander"): cv.use_id(PicoUARTExpanderComponent),
        cv.Required(CONF_NUMBER): cv.int_range(min=1, max=15),
    }
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    parent = await cg.get_variable(config["pico_uart_expander"])
    cg.add(var.set_parent(parent))
    cg.add(var.set_channel(config[CONF_NUMBER]))
    await output.register_output(var, config)
