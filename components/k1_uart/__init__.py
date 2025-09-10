import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

k1_uart_ns = cg.esphome_ns.namespace("k1_uart")
K1UartComponent = k1_uart_ns.class_("K1UartComponent", cg.Component)

# Optional buzzer linkage
buzzer_ns = cg.esphome_ns.namespace("buzzer")
BuzzerComponent = buzzer_ns.class_("BuzzerComponent", cg.Component)

CONF_BUZZER_ID = "buzzer_id"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(K1UartComponent),
        cv.Optional(CONF_BUZZER_ID): cv.use_id(BuzzerComponent),
    }
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    if CONF_BUZZER_ID in config:
        buz = await cg.get_variable(config[CONF_BUZZER_ID])
        cg.add(var.set_buzzer(buz))
    await cg.register_component(var, config)
