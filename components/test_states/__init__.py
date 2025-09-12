import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.const import CONF_ID

# Namespace
test_states_ns = cg.esphome_ns.namespace("test_states")

# Classes defined in C++
TestStatesComponent = test_states_ns.class_("TestStatesComponent", cg.Component)
ModeTrigger = test_states_ns.class_("ModeTrigger", automation.Trigger)
SetModeAction = test_states_ns.class_("SetModeAction", automation.Action)

CONF_MODES = "modes"
CONF_MODE = "mode"

MULTI_CONF = True

# Each mode name maps to ONE automation (single=True) -> user provides an automation dict
# Example:
# modes:
#   incorrect_pin:
#     then:
#       - logger.log: "Handling incorrect pin"
MODES_SCHEMA = cv.Schema(
    {
        cv.string: automation.validate_automation(single=True)
    }
)

COMPONENT_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(TestStatesComponent),
        cv.Optional(CONF_MODES, default={}): MODES_SCHEMA,
    }
).extend(cv.COMPONENT_SCHEMA)

CONFIG_SCHEMA = cv.All(cv.ensure_list(COMPONENT_SCHEMA))

@automation.register_action(
    "test_states.set_mode",
    SetModeAction,
    cv.Schema(
        {
            cv.Required(CONF_ID): cv.use_id(TestStatesComponent),
            cv.Required(CONF_MODE): cv.templatable(cv.string_strict),
        }
    ),
)
async def test_states_set_mode_action_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    action = cg.new_Pvariable(action_id, template_arg, parent)
    mode_expr = await cg.templatable(config[CONF_MODE], args, cg.std_string)
    cg.add(action.set_mode(mode_expr))
    return action


async def to_code(config):
    for comp_conf in config:
        var = cg.new_Pvariable(comp_conf[CONF_ID])
        await cg.register_component(var, comp_conf)

        modes = comp_conf.get(CONF_MODES, {})
        for mode_name, auto_list in modes.items():
            # auto_list is a list with exactly one automation dict (because single=True)
            auto_conf = auto_list[0]
            trig = cg.new_Pvariable(auto_conf[automation.CONF_TRIGGER_ID], var)
            cg.add(var.add_mode_trigger(mode_name, trig))
            await automation.build_automation(trig, [], auto_conf)
