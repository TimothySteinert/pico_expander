import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

test_states_ns = cg.esphome_ns.namespace("test_states")
TestStatesComponent = test_states_ns.class_("TestStatesComponent", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(TestStatesComponent),
    }
).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
