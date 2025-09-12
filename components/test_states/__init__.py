import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.const import CONF_ID

test_states_ns = cg.esphome_ns.namespace("test_states")

TestStatesComponent = test_states_ns.class_("TestStatesComponent", cg.Component)
ModeTrigger = test_states_ns.class_("ModeTrigger", automation.Trigger)

STATE_KEYS = ["mode1", "mode2", "mode3"]

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(TestStatesComponent),
        cv.Optional("mode1"): automation.validate_automation(single=False),
        cv.Optional("mode2"): automation.validate_automation(single=False),
        cv.Optional("mode3"): automation.validate_automation(single=False),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    # For each configured mode block, create a trigger with a fresh internal ID
    for key in STATE_KEYS:
        if key in config:
            trig_id = cg.generate_id(f"{config[CONF_ID].id}_{key}_trig")
            trig = cg.new_Pvariable(trig_id, ModeTrigger)
            # Attach the trigger to the component
            cg.add(getattr(var, f"set_{key}_trigger")(trig))
            # Build each automation entry onto this trigger
            for conf in config[key]:
                await automation.build_automation(trig, [], conf)
