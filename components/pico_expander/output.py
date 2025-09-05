import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import output
from esphome.const import CONF_ID, CONF_NUMBER

from . import PicoExpanderComponent, CONF_PICO_EXPANDER

pico_expander_ns = cg.esphome_ns.namespace("pico_expander")

PicoExpanderOutput = pico_expander_ns.class_("PicoExpanderOutput", output.FloatOutput)
PicoExpanderGPIOOutput = pico_expander_ns.class_("PicoExpanderGPIOOutput", output.BinaryOutput)

# Allow either a FLOAT (with number 0x30–0x3E) or a GPIO (fixed at 0x40)
CONFIG_SCHEMA = cv.typed_schema(
    {
        "float": output.FLOAT_OUTPUT_SCHEMA.extend(
            {
                cv.GenerateID(CONF_ID): cv.declare_id(PicoExpanderOutput),
                cv.Required(CONF_PICO_EXPANDER): cv.use_id(PicoExpanderComponent),
                cv.Required(CONF_NUMBER): cv.int_range(min=0x30, max=0x3E),
            }
        ),
        "gpio": output.BINARY_OUTPUT_SCHEMA.extend(
            {
                cv.GenerateID(CONF_ID): cv.declare_id(PicoExpanderGPIOOutput),
                cv.Required(CONF_PICO_EXPANDER): cv.use_id(PicoExpanderComponent),
            }
        ),
    }
)

async def to_code(config):
    if config["type"] == "float":
        var = cg.new_Pvariable(config[CONF_ID])
        parent = await cg.get_variable(config[CONF_PICO_EXPANDER])
        cg.add(var.set_parent(parent))
        cg.add(var.set_channel(config[CONF_NUMBER]))
        await output.register_output(var, config)
    elif config["type"] == "gpio":
        var = cg.new_Pvariable(config[CONF_ID])
        parent = await cg.get_variable(config[CONF_PICO_EXPANDER])
        cg.add(var.set_parent(parent))
        await output.register_output(var, config)
