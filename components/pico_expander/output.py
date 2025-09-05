import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import output
from esphome.const import CONF_ID, CONF_NUMBER

from . import (
    PicoExpanderComponent,
    CONF_PICO_EXPANDER,
    PicoExpanderLedOutput,
    PicoExpanderBuzzerOutput,
)

# --- LED Outputs (0x30–0x3E) ---
CONFIG_SCHEMA_LED = output.FLOAT_OUTPUT_SCHEMA.extend(
    {
        cv.GenerateID(CONF_ID): cv.declare_id(PicoExpanderLedOutput),
        cv.Required(CONF_PICO_EXPANDER): cv.use_id(PicoExpanderComponent),
        cv.Required(CONF_NUMBER): cv.int_range(min=0x30, max=0x3E),
    }
)

@output.register_output("pico_expander_led", PicoExpanderLedOutput, CONFIG_SCHEMA_LED)
async def led_to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    parent = await cg.get_variable(config[CONF_PICO_EXPANDER])
    cg.add(var.set_parent(parent))
    cg.add(var.set_channel(config[CONF_NUMBER]))
    await output.register_output(var, config)


# --- Buzzer Output (0x40 duty + 0x41/0x42 frequency) ---
CONFIG_SCHEMA_BUZZER = output.FLOAT_OUTPUT_SCHEMA.extend(
    {
        cv.GenerateID(CONF_ID): cv.declare_id(PicoExpanderBuzzerOutput),
        cv.Required(CONF_PICO_EXPANDER): cv.use_id(PicoExpanderComponent),
        cv.Required(CONF_NUMBER): cv.int_range(min=0x40, max=0x40),  # duty only
    }
)

@output.register_output("pico_expander_buzzer", PicoExpanderBuzzerOutput, CONFIG_SCHEMA_BUZZER)
async def buzzer_to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    parent = await cg.get_variable(config[CONF_PICO_EXPANDER])
    cg.add(var.set_parent(parent))
    cg.add(var.set_channel(config[CONF_NUMBER]))
    await output.register_output(var, config)
