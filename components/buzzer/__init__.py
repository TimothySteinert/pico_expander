import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_OUTPUT
from esphome import automation

output_ns = cg.esphome_ns.namespace("output")
FloatOutput = output_ns.class_("FloatOutput", cg.Component)

buzzer_ns = cg.esphome_ns.namespace("buzzer")
BuzzerComponent = buzzer_ns.class_("BuzzerComponent", cg.Component)
StartAction = buzzer_ns.class_("StartAction", automation.Action)
StopAction = buzzer_ns.class_("StopAction", automation.Action)

CONF_BEEPS = "beeps"
CONF_SHORT_PAUSE = "short_pause"
CONF_LONG_PAUSE = "long_pause"
CONF_TONE = "tone"
CONF_REPEAT = "repeat"
CONF_BEEP_LENGTH = "beep_length"

CONFIG_SCHEMA = cv.Schema({
    cv.Required(CONF_ID): cv.declare_id(BuzzerComponent),
    cv.Required(CONF_OUTPUT): cv.use_id(FloatOutput),
})

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    out = await cg.get_variable(config[CONF_OUTPUT])
    cg.add(var.set_output(out))

@automation.register_action(
    "buzzer.start",
    StartAction,
    cv.Schema(
        {
            cv.Required(CONF_ID): cv.use_id(BuzzerComponent),
            cv.Required(CONF_BEEPS): cv.templatable(cv.uint8_t),
            cv.Required(CONF_SHORT_PAUSE): cv.templatable(cv.positive_time_period_milliseconds),
            cv.Required(CONF_LONG_PAUSE): cv.templatable(cv.positive_time_period_milliseconds),
            cv.Required(CONF_TONE): cv.templatable(cv.uint8_t),
            cv.Required(CONF_REPEAT): cv.templatable(cv.boolean),
            cv.Optional(CONF_BEEP_LENGTH, default="200ms"): cv.templatable(
                cv.positive_time_period_milliseconds
            ),
        }
    ),
)
async def buzzer_start_to_code(config, action_id, template_arg, args):
    var = await cg.get_variable(config[CONF_ID])
    action = StartAction(var)
    cg.add(action.beeps(config[CONF_BEEPS]))
    cg.add(action.short_pause(config[CONF_SHORT_PAUSE]))
    cg.add(action.long_pause(config[CONF_LONG_PAUSE]))
    cg.add(action.tone(config[CONF_TONE]))
    cg.add(action.repeat(config[CONF_REPEAT]))
    cg.add(action.beep_length(config[CONF_BEEP_LENGTH]))
    return action

@automation.register_action(
    "buzzer.stop",
    StopAction,
    cv.Schema({cv.Required(CONF_ID): cv.use_id(BuzzerComponent)}),
)
async def buzzer_stop_to_code(config, action_id, template_arg, args):
    var = await cg.get_variable(config[CONF_ID])
    return StopAction(var)
