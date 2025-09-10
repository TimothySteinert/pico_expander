import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import CONF_ID
from esphome import automation

buzzer_ns = cg.esphome_ns.namespace("buzzer")
BuzzerComponent = buzzer_ns.class_("BuzzerComponent", cg.Component)
BuzzerMuteSwitch = buzzer_ns.class_("BuzzerMuteSwitch", switch.Switch, cg.Component)

CONF_BUZZER_ID = "buzzer_id"
CONF_TYPE = "type"

MUTE_TYPES = ["tone_mute", "beep_mute"]

CONFIG_SCHEMA = switch.switch_schema(BuzzerMuteSwitch).extend(
    {
        cv.Required(CONF_BUZZER_ID): cv.use_id(BuzzerComponent),
        cv.Required(CONF_TYPE): cv.one_of(*MUTE_TYPES, lower=True),
    }
)

async def to_code(config):
    parent = await cg.get_variable(config[CONF_BUZZER_ID])
    # Create the switch (constructor takes parent)
    var = cg.new_Pvariable(config[CONF_ID], parent)

    # Set which mute type (0 = tone, 1 = beep)
    type_enum = 0 if config[CONF_TYPE] == "tone_mute" else 1
    cg.add(var.set_type(type_enum))

    # Register as component then as switch
    await cg.register_component(var, config)
    await switch.register_switch(var, config)

    # Let parent keep pointer so it can sync states
    if config[CONF_TYPE] == "tone_mute":
        cg.add(parent.register_tone_switch(var))
    else:
        cg.add(parent.register_beep_switch(var))
