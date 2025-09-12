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

MODE_KEYS = ["mode1", "mode2", "mode3", "mode4", "mode5"]

MULTI_CONF = True

# Build a validator for an automation whose trigger is ModeTrigger
MODE_AUTOMATION = automation.validate_automation({
    cv.GenerateID(automation.CONF_TRIGGER_ID): cv.declare_id(ModeTrigger),
})

# Each mode key may have a list of automation blocks
SCHEMA_MODES = {
    cv.Optional(mk): cv.ensure_list(MODE_AUTOMATION) for mk in MODE_KEYS
}

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

        first_defined = None
        for mk in MODE_KEYS:
            blocks = comp_conf.get(mk, [])
            if blocks and first_defined is None:
                first_defined = mk
            for trig_conf in blocks:
                trig = cg.new_Pvariable(trig_conf[automation.CONF_TRIGGER_ID])
                cg.add(getattr(var, f"add_{mk}_trigger")(trig))
                await automation.build_automation(trig, [], trig_conf)

        initial_mode = comp_conf.get(CONF_INITIAL_MODE)
        if initial_mode is not None:
            cg.add(var.set_initial_mode(initial_mode))
        elif first_defined is not None:
            cg.add(var.set_initial_mode(first_defined))
        else:
            cg.add(var.set_initial_mode("mode1"))
