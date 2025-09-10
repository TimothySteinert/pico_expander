import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

k1_uart_ns = cg.esphome_ns.namespace("k1_uart")
K1UartComponent = k1_uart_ns.class_("K1UartComponent", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(K1UartComponent),
    }
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
