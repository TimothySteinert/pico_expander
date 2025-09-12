import esphome.codegen as cg
import esphome.config_validation as cv

from esphome.components import api
from esphome.const import CONF_ID

k1_arm_handler_ns = cg.esphome_ns.namespace("k1_arm_handler")
K1ArmHandlerComponent = k1_arm_handler_ns.class_(
    "K1ArmHandlerComponent",
    cg.Component,
    api.CustomAPIDevice
)

CONF_ALARM_ENTITY_ID = "alarm_entity_id"
CONF_ARM_SERVICE = "arm_service"
CONF_DISARM_SERVICE = "disarm_service"
CONF_FORCE_PREFIX = "force_prefix"
CONF_SKIP_DELAY_PREFIX = "skip_delay_prefix"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(K1ArmHandlerComponent),
        cv.Required(CONF_ALARM_ENTITY_ID): cv.entity_id,
        cv.Optional(CONF_ARM_SERVICE, default="alarmo.arm"): cv.string,
        cv.Optional(CONF_DISARM_SERVICE, default="alarm_control_panel.alarm_disarm"): cv.string,
        cv.Optional(CONF_FORCE_PREFIX, default="999"): cv.string,
        cv.Optional(CONF_SKIP_DELAY_PREFIX, default="998"): cv.string,
    }
).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.set_alarm_entity_id(config[CONF_ALARM_ENTITY_ID]))
    cg.add(var.set_arm_service(config[CONF_ARM_SERVICE]))
    cg.add(var.set_disarm_service(config[CONF_DISARM_SERVICE]))
    cg.add(var.set_force_prefix(config[CONF_FORCE_PREFIX]))
    cg.add(var.set_skip_delay_prefix(config[CONF_SKIP_DELAY_PREFIX]))
