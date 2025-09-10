import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID

CONF_PICO_UART_EXPANDER = "pico_uart_expander"

DEPENDENCIES = ["uart"]
MULTI_CONF = True

pico_uart_expander_ns = cg.esphome_ns.namespace("pico_uart_expander")

PicoUartExpanderComponent = pico_uart_expander_ns.class_(
    "PicoUartExpanderComponent", cg.Component, uart.UARTDevice
)

CONFIG_SCHEMA = (
    cv.Schema({cv.Required(CONF_ID): cv.declare_id(PicoUartExpanderComponent)})
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
