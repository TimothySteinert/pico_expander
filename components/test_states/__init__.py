import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.const import CONF_ID

test_states_ns = cg.esphome_ns.namespace("test_states")

TestStatesComponent = test_states_ns.class_("TestStatesComponent", cg.Component)
ModeTrigger = test_states_ns.class_("ModeTrigger", automation.Trigger)
SetModeAction = test_states_ns.class_("SetModeAction", automation.Action)

CONF_MODES = "modes"
CONF_MODE = "mode"

MULTI_CONF = True

# Accept:
# modes:
#   mode1:
#     - logger.log: "short form action list"
#   mode2:
#     - then:
#         - logger.log: "full automation"
#   mode3:
#     - then:
#         - logger.log: "first block"
#     - then:
#         - logger.log: "second block"
#
# Internally we normalize every mode's value to a list of automation dicts
# each containing a 'then:' list.
def _modes_validator(value):
    if not isinstance(value, dict):
        raise cv.Invalid("modes: must be a mapping of mode_name -> actions")
    out = {}
    for mode_name, raw in value.items():
        # Ensure list
        if not isinstance(raw, list):
            # Could be a single automation dict, or a single action
            if isinstance(raw, dict) and "then" in raw:
                raw_list = [raw]
            else:
                # Single action dict -> wrap
                raw_list = [ {"then": [raw]} ]
        else:
            # raw is a list. Determine if it's already a list of automation dicts or a list of plain actions.
            treat_as_plain_actions = True
            for item in raw:
                if isinstance(item, dict) and "then" in item:
                    treat_as_plain_actions = False
                    break
            if treat_as_plain_actions:
                # Entire list are plain actions -> wrap into one automation block
                raw_list = [ {"then": raw} ]
            else:
                # Already list of automation dicts (each must have then)
                raw_list = raw

        # Validate each automation block with standard automation schema
        norm_blocks = []
        for idx, blk in enumerate(raw_list):
            if not isinstance(blk, dict):
                raise cv.Invalid(f"Mode '{mode_name}' entry #{idx} must be a dict.")
            if "then" not in blk:
                raise cv.Invalid(f"Mode '{mode_name}' automation #{idx} missing 'then:' section")
            # Let automation.validate_automation() process it (returns list with trigger dicts)
            validated_list = automation.validate_automation()(blk)
            # That returns a list; we append its (single) element
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
                trig = cg.new_Pvariable(auto_conf[automation.CONF_TRIGGER_ID], var)
                cg.add(var.add_mode_trigger(mode_name, trig))
                await automation.build_automation(trig, [], auto_conf)
        # Initialize starting mode to the first one declared (if any)
        if first_mode is not None:
            cg.add(var.set_initial_mode(first_mode))
