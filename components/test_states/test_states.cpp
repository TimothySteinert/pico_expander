import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.const import CONF_ID

test_states_ns = cg.esphome_ns.namespace("test_states")

TestStatesComponent = test_states_ns.class_("TestStatesComponent", cg.Component)
ModeTrigger = test_states_ns.class_("ModeTrigger", automation.Trigger)
PreModeTrigger = test_states_ns.class_("PreModeTrigger", automation.Trigger)
SetModeAction = test_states_ns.class_("SetModeAction", automation.Action)

CONF_INITIAL_MODE = "initial_mode"
CONF_MODE = "mode"
CONF_INTERMODE = "intermode"

# Full fixed mode list
MODE_KEYS = [
    "disarmed",
    "triggered",
    "connection_timeout",
    "incorrect_pin",
    "failed_open_sensors",

    "arming",
    "arming_home",
    "arming_away",
    "arming_night",
    "arming_vacation",
    "arming_custom_bypass",

    "pending",
    "pending_home",
    "pending_away",
    "pending_night",
    "pending_vacation",
    "pending_custom_bypass",

    "armed_home",
    "armed_away",
    "armed_night",
    "armed_vacation",

    "armed_home_bypass",
    "armed_away_bypass",
    "armed_night_bypass",
    "armed_vacation_bypass",
    "armed_custom_bypass",
]

MULTI_CONF = True

MODE_AUTOMATION = automation.validate_automation({
    cv.GenerateID(automation.CONF_TRIGGER_ID): cv.declare_id(ModeTrigger),
})
INTERMODE_AUTOMATION = automation.validate_automation({
    cv.GenerateID(automation.CONF_TRIGGER_ID): cv.declare_id(PreModeTrigger),
})

def _automation_blocks(validator):
    def inner(value):
        flat = []
        if isinstance(value, list):
            for item in value:
                flat.extend(validator(item))
        else:
            flat.extend(validator(value))
        return flat
    return inner

mode_blocks_validator = _automation_blocks(MODE_AUTOMATION)
intermode_blocks_validator = _automation_blocks(INTERMODE_AUTOMATION)

SCHEMA_MODES = {cv.Optional(mk): mode_blocks_validator for mk in MODE_KEYS}

COMPONENT_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(TestStatesComponent),
        cv.Optional(CONF_INITIAL_MODE): cv.one_of(*MODE_KEYS, lower=True),
        cv.Optional(CONF_INTERMODE): intermode_blocks_validator,
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

        # Intermode triggers (run before every real transition)
        inter_trigs = comp_conf.get(CONF_INTERMODE, [])
        for trig_conf in inter_trigs:
            ptr = cg.new_Pvariable(trig_conf[automation.CONF_TRIGGER_ID])
            cg.add(var.add_inter_mode_trigger(ptr))
            await automation.build_automation(ptr, [], trig_conf)

        first_defined = None
        for mk in MODE_KEYS:
            triggers = comp_conf.get(mk, [])
            if triggers and first_defined is None:
                first_defined = mk
            for trig_conf in triggers:
                trig = cg.new_Pvariable(trig_conf[automation.CONF_TRIGGER_ID])
                # add_<mode>_trigger
                cg.add(getattr(var, f"add_{mk}_trigger")(trig))
                await automation.build_automation(trig, [], trig_conf)

        initial_mode = comp_conf.get(CONF_INITIAL_MODE)
        if initial_mode is not None:
            cg.add(var.set_initial_mode(initial_mode))
        elif first_defined is not None:
            cg.add(var.set_initial_mode(first_defined))
        else:
            cg.add(var.set_initial_mode("disarmed"))
