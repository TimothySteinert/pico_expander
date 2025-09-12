import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import script

k1_arm_handler_ns = cg.esphome_ns.namespace("k1_arm_handler")
K1ArmHandlerComponent = k1_arm_handler_ns.class_("K1ArmHandlerComponent", cg.Component)

CONF_ID = "id"
CONF_ALARM_ENTITY_ID = "alarm_entity_id"     # kept for reference/logging (not used internally now)
CONF_FORCE_PREFIX = "force_prefix"
CONF_SKIP_DELAY_PREFIX = "skip_delay_prefix"
CONF_CALLBACK_SCRIPT = "callback_script"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(K1ArmHandlerComponent),
        cv.Optional(CONF_ALARM_ENTITY_ID, default=""): cv.string,
        cv.Optional(CONF_FORCE_PREFIX, default="999"): cv.string,
        cv.Optional(CONF_SKIP_DELAY_PREFIX, default="998"): cv.string,
        cv.Required(CONF_CALLBACK_SCRIPT): cv.use_id(script.Script),
    }
).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.set_alarm_entity_id(config[CONF_ALARM_ENTITY_ID]))
    cg.add(var.set_force_prefix(config[CONF_FORCE_PREFIX]))
    cg.add(var.set_skip_delay_prefix(config[CONF_SKIP_DELAY_PREFIX]))

    scr = await cg.get_variable(config[CONF_CALLBACK_SCRIPT])
    cg.add(var.set_callback_script(scr))
