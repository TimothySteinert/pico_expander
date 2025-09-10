import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import output
from esphome.const import CONF_ID, CONF_NUMBER

from . import PicoUartExpanderComponent, CONF_PICO_UART_EXPANDER

pico_uart_expander_ns = cg.esphome_ns.namespace("pico_uart_expander")
PicoUartExpanderOutput = pico_uart_expander_ns.class_("PicoUartExpanderOutput", output.FloatOutput)

CONFIG_SCHEMA = output.FLOAT_OUTPUT_SCHEMA.extend(
    {
        cv.GenerateID(CONF_ID): cv.declare_id(PicoUartExpanderOutput),
        cv.Required(CONF_PICO_UART_EXPANDER): cv.use_id(PicoUartExpanderComponent),
        cv.Required(CONF_NUMBER): cv.int_range(min=1, max=16),  # 1-15 for LEDs, 16 for buzzer
    }
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    parent = await cg.get_variable(config[CONF_PICO_UART_EXPANDER])
    cg.add(var.set_parent(parent))
    cg.add(var.set_channel(config[CONF_NUMBER]))
    await output.register_output(var, config)
