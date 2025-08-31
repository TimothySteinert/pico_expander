import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import output
from esphome.const import CONF_ID, CONF_NUMBER

from . import (
    PicoExpanderComponent,
    PicoExpanderOutput,
    CONF_PICO_EXPANDER,
)

# Platform module for:
# output:
#   - platform: pico_expander
CONFIG_SCHEMA = output.FLOAT_OUTPUT_SCHEMA.extend(
    {
        cv.GenerateID(CONF_ID): cv.declare_id(PicoExpanderOutput),
        cv.Required(CONF_PICO_EXPANDER): cv.use_id(PicoExpanderComponent),
        cv.Required(CONF_NUMBER): cv.int_range(min=0, max=2),
    }
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    parent = await cg.get_variable(config[CONF_PICO_EXPANDER])
    cg.add(var.set_parent(parent))
    cg.add(var.set_channel(config[CONF_NUMBER]))
    # This applies min_power/max_power/zero_means_zero/inverted/power_supply
    await output.register_output(var, config)
