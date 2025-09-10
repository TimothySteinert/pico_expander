import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import output
from esphome.const import CONF_ID

from . import argb_strip_ns, ARGBStripComponent

CONF_ARGB_STRIP_ID = "argb_strip_id"
CONF_GROUP = "group"
CONF_CHANNEL = "channel"

COLOR_CHANNELS = {
    "red": 0,
    "green": 1,
    "blue": 2,
}

ARGBStripOutput = argb_strip_ns.class_("ARGBStripOutput", output.FloatOutput, cg.Component)

CONFIG_SCHEMA = output.FLOAT_OUTPUT_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(ARGBStripOutput),
        cv.GenerateID(CONF_ARGB_STRIP_ID): cv.use_id(ARGBStripComponent),
        cv.Required(CONF_GROUP): cv.valid_name,
        cv.Required(CONF_CHANNEL): cv.enum(COLOR_CHANNELS, lower=True),
    }
).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    parent = await cg.get_variable(config[CONF_ARGB_STRIP_ID])
    var = cg.new_Pvariable(config[CONF_ID])
    await output.register_output(var, config)
    await cg.register_component(var, config)

    cg.add(var.set_parent(parent))
    cg.add(var.set_group_name(config[CONF_GROUP]))
    cg.add(var.set_channel(config[CHANNEL]))
