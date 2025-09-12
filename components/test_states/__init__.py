import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.const import CONF_ID

test_states_ns = cg.esphome_ns.namespace("test_states")

TestStatesComponent = test_states_ns.class_("TestStatesComponent", cg.Component)
ModeTrigger = test_states_ns.class_("ModeTrigger", automation.Trigger)
SetModeAction = test_states_ns.class_("SetModeAction", automation.Action)

CONF_INITIAL_MODE = "initial_mode"
CONF_MODE = "mode"

# Hard-coded mode keys
MODE_KEYS = ["mode1", "mode2", "mode3", "mode4", "mode5"]

MULTI_CONF = True

# Define our own automation block schema so we control the trigger type.
# Each block:
#   - auto-generated trigger id (ModeTrigger)
#   - required 'then' list of standard actions
MODE_AUTOMATION_BLOCK = cv.Schema(
    {
        cv.GenerateID(automation.CONF_TRIGGER_ID): cv.declare_id(ModeTrigger),
        cv.Required("then"): cv.ensure_list(automation.ACTION_SCHEMA),
    }
)

def _mode_list_schema(value):
    # Allow either a single block or a list of blocks
    if isinstance(value, list):
        return [MODE_AUTOMATION_BLOCK(v) for v in value]
    return [MODE_AUTOMATION_BLOCK(value)]

SCHEMA_MODES = {cv.Optional(mk): _mode_list_schema for mk in MODE_KEYS}

COMPONENT_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(TestStatesComponent),
        cv.Optional(CONF_INITIAL_MODE): cv.one_of(*MODE_KEYS, lower=True),
        **SCHEMA_MODES,
    }
).extend(cv.COMPONENT_SCHEMA)

CONFIG_SCHEMA = cv.All(cv.ensure_list(COMPONENT_SCHEMA))

@automation.register_action(
    "test_states.set_mode",
    SetModeAction,
    cv.Schema(
        {
            cv.Required(CONF_ID): cv.use_id(TestStatesComponent),
            cv.Required(CONF_MODE): cv.templatable(cv.one_of(*MODE_KEYS, lower=True)),
        }
    ),
)
async def test_states_set_mode_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    action = cg.new_Pvariable(action_id, template_arg, parent)
    mode_expr = await cg.templatable(config[CONF_MODE], args, cg.std_string)
    cg.add(action.set_mode(mode_expr))
    return action


async def to_code(config):
    for comp_conf in config:
        var = cg.new_Pvariable(comp_conf[CONF_ID])
        await cg.register_component(var, comp_conf)

        initial_mode = comp_conf.get(CONF_INITIAL_MODE)
        first_defined = None

        for mk in MODE_KEYS:
            if mk in comp_conf:
                if first_defined is None:
                    first_defined = mk
                for block in comp_conf[mk]:
                    trig = cg.new_Pvariable(block[automation.CONF_TRIGGER_ID])
                    cg.add(getattr(var, f"add_{mk}_trigger")(trig))
                    # Build the automation (no args passed to trigger)
                    await automation.build_automation(trig, [], block)

        if initial_mode is not None:
            cg.add(var.set_initial_mode(initial_mode))
        elif first_defined is not None:
            cg.add(var.set_initial_mode(first_defined))
        else:
            cg.add(var.set_initial_mode("mode1"))
