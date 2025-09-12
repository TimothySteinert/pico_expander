import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.const import CONF_ID

# Namespace & classes
test_states_ns = cg.esphome_ns.namespace("test_states")

TestStatesComponent = test_states_ns.class_("TestStatesComponent", cg.Component)
ModeTrigger = test_states_ns.class_("ModeTrigger", automation.Trigger)
SetModeAction = test_states_ns.class_("SetModeAction", automation.Action)

CONF_MODES = "modes"
CONF_MODE = "mode"

MULTI_CONF = True

# Validator for modes:
# Supports:
#   modes:
#     mode1:
#       - logger.log: "short form list of actions"
#     mode2:
#       - then:
#           - logger.log: "full automation block"
#       - then:
#           - logger.log: "second automation block"
#     mode3:
#       - logger.log: "single action short form"
#
# Internally normalized to a list of automation dicts (each with then:)
MODE_AUTOMATION = automation.validate_automation(ModeTrigger)

def _modes_validator(value):
    if not isinstance(value, dict):
        raise cv.Invalid("modes must be a mapping of mode_name -> action list / automation blocks")
    out = {}
    for mode_name, raw in value.items():
        # Normalize raw into list
        if not isinstance(raw, list):
            # Single item (action or automation)
            if isinstance(raw, dict) and "then" in raw:
                raw_list = [raw]
            else:
                raw_list = [{"then": [raw]}]
        else:
            # List given. Detect if this is a plain action list (no 'then' keys)
            treat_as_plain_actions = True
            for item in raw:
                if isinstance(item, dict) and "then" in item:
                    treat_as_plain_actions = False
                    break
            if treat_as_plain_actions:
                raw_list = [{"then": raw}]
            else:
                raw_list = raw

        norm_blocks = []
        for idx, blk in enumerate(raw_list):
            if not isinstance(blk, dict):
                raise cv.Invalid(f"Mode '{mode_name}' entry #{idx} must be a dict")
            if "then" not in blk:
                raise cv.Invalid(f"Mode '{mode_name}' automation #{idx} missing 'then:'")
            # Validate & generate trigger id etc.
            validated_list = MODE_AUTOMATION(blk)  # returns list with 1 element
            norm_blocks.extend(validated_list)
        out[mode_name] = norm_blocks
    return out

COMPONENT_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(TestStatesComponent),
        cv.Optional(CONF_MODES, default={}): _modes_validator,
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
        first_mode = None
        for mode_name, automation_list in modes.items():
            if first_mode is None:
                first_mode = mode_name
            for auto_conf in automation_list:
                trig = cg.new_Pvariable(auto_conf[automation.CONF_TRIGGER_ID])
                cg.add(var.add_mode_trigger(mode_name, trig))
                await automation.build_automation(trig, [], auto_conf)
        if first_mode is not None:
            cg.add(var.set_initial_mode(first_mode))
