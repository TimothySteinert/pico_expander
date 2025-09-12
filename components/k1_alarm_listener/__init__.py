import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

k1_alarm_listener_ns = cg.esphome_ns.namespace("k1_alarm_listener")
K1AlarmListener = k1_alarm_listener_ns.class_("K1AlarmListener", cg.Component)

CONF_ALARM_ENTITY = "alarm_entity"
CONF_ALARM_TYPE = "alarm_type"

ALARM_TYPES = {
    "alarmo": 0,
    "other": 1,
}

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(K1AlarmListener),
        cv.Required(CONF_ALARM_ENTITY): cv.string,
        cv.Optional(CONF_ALARM_TYPE, default="alarmo"): cv.one_of(*ALARM_TYPES.keys(), lower=True),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_alarm_entity(config[CONF_ALARM_ENTITY]))
    cg.add(var.set_alarm_type(ALARM_TYPES[config[CONF_ALARM_TYPE]]))
