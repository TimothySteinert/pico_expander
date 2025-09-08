import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID, CONF_NUMBER

DEPENDENCIES = ["uart"]

CONF_PICO_UART_EXPANDER = "pico_uart_expander"

pico_uart_expander_ns = cg.esphome_ns.namespace("pico_uart_expander")

PicoUARTExpanderComponent = pico_uart_expander_ns.class_(
    "PicoUARTExpanderComponent", cg.Component, uart.UARTDevice
)

PicoUARTExpanderOutput = pico_uart_expander_ns.class_(
    "PicoUARTExpanderOutput", cg.Component, cg.output.FloatOutput
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(CONF_ID): cv.declare_id(PicoUARTExpanderComponent),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)

OUTPUT_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ID): cv.declare_id(PicoUARTExpanderOutput),
        cv.Required(CONF_PICO_UART_EXPANDER): cv.use_id(PicoUARTExpanderComponent),
        cv.Required(CONF_NUMBER): cv.int_range(min=1, max=15),
    }
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

async def to_code_output(config):
    var = cg.new_Pvariable(config[CONF_ID])
    parent = await cg.get_variable(config[CONF_PICO_UART_EXPANDER])
    cg.add(var.set_parent(parent))
    cg.add(var.set_channel(config[CONF_NUMBER]))
    await cg.register_component(var, config)
    await cg.register_output(var, config)
