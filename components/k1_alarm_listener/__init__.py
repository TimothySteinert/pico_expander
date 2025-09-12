import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

k1_alarm_listener_ns = cg.esphome_ns.namespace("k1_alarm_listener")
K1AlarmListener = k1_alarm_listener_ns.class_("K1AlarmListener", cg.Component)

CONF_ALARM_ENTITY = "alarm_entity"

DEPENDENCIES = ["api"]

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(K1AlarmListener),
        cv.Required(CONF_ALARM_ENTITY): cv.string,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_alarm_entity(config[CONF_ALARM_ENTITY]))
