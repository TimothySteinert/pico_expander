import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.const import CONF_ID

test_states_ns = cg.esphome_ns.namespace("test_states")

TestStatesComponent = test_states_ns.class_("TestStatesComponent", cg.Component)
ModeTrigger = test_states_ns.class_("ModeTrigger", automation.Trigger)

STATE_KEYS = ["mode1", "mode2", "mode3"]

# We allow exactly one automation block per mode (single=True). User supplies a plain list of actions.
CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(TestStatesComponent),
        cv.Optional("mode1"): automation.validate_automation(single=True),
        cv.Optional("mode2"): automation.validate_automation(single=True),
        cv.Optional("mode3"): automation.validate_automation(single=True),
    }
).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    # For each mode that has an automation block, create a ModeTrigger and attach it.
    # automation.validate_automation(single=True) returns a list (0 or 1 items)
    for key in STATE_KEYS:
        for conf in config.get(key, []):
            # Create a trigger (constructor takes parent component)
            trig = cg.new_Pvariable(conf[automation.CONF_TRIGGER_ID], var)
            cg.add(getattr(var, f"add_{key}_trigger")(trig))
            await automation.build_automation(trig, [], conf)
