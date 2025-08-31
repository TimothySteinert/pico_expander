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
PicoExpanderOutput = pico_expander_ns.class_("PicoExpanderOutput", output.Output)

CONFIG_SCHEMA = (
    cv.Schema({cv.Required(CONF_ID): cv.declare_id(PicoExpanderComponent)})
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x08))
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

# Output schema
OUTPUT_SCHEMA = output.FLOAT_OUTPUT_SCHEMA.extend(
    {
        cv.GenerateID(CONF_ID): cv.declare_id(PicoExpanderOutput),
        cv.Required(CONF_PICO_EXPANDER): cv.use_id(PicoExpanderComponent),
        cv.Required(CONF_NUMBER): cv.int_range(min=0, max=2),
    }
)

@output.register_output("pico_expander", PicoExpanderOutput, OUTPUT_SCHEMA)
async def output_to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])  # <- this ID is for PicoExpanderOutput now
    parent = await cg.get_variable(config[CONF_PICO_EXPANDER])
    cg.add(var.set_parent(parent))
    cg.add(var.set_channel(config[CONF_NUMBER]))
    await output.register_output(var, config)
