import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c, output
from esphome.const import CONF_ID, CONF_NUMBER

CONF_PICO_EXPANDER = "pico_expander"

DEPENDENCIES = ["i2c"]
MULTI_CONF = True

pico_expander_ns = cg.esphome_ns.namespace("pico_expander")

PicoExpanderComponent = pico_expander_ns.class_(
    "PicoExpanderComponent", cg.Component, i2c.I2CDevice
)
PicoExpanderOutput = pico_expander_ns.class_(
    "PicoExpanderOutput", output.FloatOutput
)

# ---------------------------------------------------------------------------
# Hub (I²C expander) config schema
# ---------------------------------------------------------------------------
CONFIG_SCHEMA = (
    cv.Schema({cv.Required(CONF_ID): cv.declare_id(PicoExpanderComponent)})
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x08))
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

# ---------------------------------------------------------------------------
# Output schema (for each RGB channel)
# FloatOutput handles min_power/max_power/zero_means_zero/inverted/etc.
# ---------------------------------------------------------------------------
OUTPUT_SCHEMA = output.FLOAT_OUTPUT_SCHEMA.extend(
    {
        cv.GenerateID(CONF_ID): cv.declare_id(PicoExpanderOutput),
        cv.Required(CONF_PICO_EXPANDER): cv.use_id(PicoExpanderComponent),
        cv.Required(CONF_NUMBER): cv.int_range(min=0, max=2),
    }
)

async def pico_expander_output_to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    parent = await cg.get_variable(config[CONF_PICO_EXPANDER])
    cg.add(var.set_parent(parent))
    cg.add(var.set_channel(config[CONF_NUMBER]))
    # Registers min/max/zero_means_zero/inverted/power_supply bindings, etc.
    await output.register_output(var, config)

# ---------------------------------------------------------------------------
# ESPHome 2025.7.x: register platform with (name, to_code)
# ---------------------------------------------------------------------------
output.setup_output_platform_("pico_expander", pico_expander_output_to_code)
