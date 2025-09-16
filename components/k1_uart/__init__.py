import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

k1_uart_ns = cg.esphome_ns.namespace("k1_uart")
K1UartComponent = k1_uart_ns.class_("K1UartComponent", cg.Component)

buzzer_ns = cg.esphome_ns.namespace("buzzer")
BuzzerComponent = buzzer_ns.class_("BuzzerComponent", cg.Component)

script_ns = cg.esphome_ns.namespace("script")
Script3 = script_ns.class_("Script", cg.Component)  # (pin, force, skip_delay)

select_ns = cg.esphome_ns.namespace("select")
SelectComponent = select_ns.class_("Select", cg.Component)

argb_ns = cg.esphome_ns.namespace("argb_strip")
ARGBStripComponent = argb_ns.class_("ARGBStripComponent", cg.Component)

CONF_BUZZER_ID = "buzzer_id"
CONF_AWAY_SCRIPT_ID = "away_script_id"
CONF_HOME_SCRIPT_ID = "home_script_id"
CONF_DISARM_SCRIPT_ID = "disarm_script_id"
CONF_NIGHT_SCRIPT_ID = "night_script_id"
CONF_VACATION_SCRIPT_ID = "vacation_script_id"
CONF_BYPASS_SCRIPT_ID = "bypass_script_id"
CONF_MODE_SELECTOR_ID = "mode_selector_id"
CONF_ARM_STRIP_ID = "arm_strip_id"

CONF_FORCE_PREFIX = "force_prefix"
CONF_SKIP_DELAY_PREFIX = "skip_delay_prefix"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(K1UartComponent),
        cv.Optional(CONF_BUZZER_ID): cv.use_id(BuzzerComponent),

        cv.Optional(CONF_AWAY_SCRIPT_ID): cv.use_id(Script3),
        cv.Optional(CONF_HOME_SCRIPT_ID): cv.use_id(Script3),
        cv.Optional(CONF_DISARM_SCRIPT_ID): cv.use_id(Script3),
        cv.Optional(CONF_NIGHT_SCRIPT_ID): cv.use_id(Script3),
        cv.Optional(CONF_VACATION_SCRIPT_ID): cv.use_id(Script3),
        cv.Optional(CONF_BYPASS_SCRIPT_ID): cv.use_id(Script3),

        cv.Optional(CONF_MODE_SELECTOR_ID): cv.use_id(SelectComponent),
        cv.Optional(CONF_ARM_STRIP_ID): cv.use_id(ARGBStripComponent),

        cv.Optional(CONF_FORCE_PREFIX, default="999"): cv.string,
        cv.Optional(CONF_SKIP_DELAY_PREFIX, default="998"): cv.string,
    }
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])

    if CONF_BUZZER_ID in config:
        buz = await cg.get_variable(config[CONF_BUZZER_ID])
        cg.add(var.set_buzzer(buz))

    # Scripts
    if CONF_AWAY_SCRIPT_ID in config:
        sc = await cg.get_variable(config[CONF_AWAY_SCRIPT_ID])
        cg.add(var.set_away_script(sc))
    if CONF_HOME_SCRIPT_ID in config:
        sc = await cg.get_variable(config[CONF_HOME_SCRIPT_ID])
        cg.add(var.set_home_script(sc))
    if CONF_DISARM_SCRIPT_ID in config:
        sc = await cg.get_variable(config[CONF_DISARM_SCRIPT_ID])
        cg.add(var.set_disarm_script(sc))
    if CONF_NIGHT_SCRIPT_ID in config:
        sc = await cg.get_variable(config[CONF_NIGHT_SCRIPT_ID])
        cg.add(var.set_night_script(sc))
    if CONF_VACATION_SCRIPT_ID in config:
        sc = await cg.get_variable(config[CONF_VACATION_SCRIPT_ID])
        cg.add(var.set_vacation_script(sc))
    if CONF_BYPASS_SCRIPT_ID in config:
        sc = await cg.get_variable(config[CONF_BYPASS_SCRIPT_ID])
        cg.add(var.set_bypass_script(sc))

    # Selector
    if CONF_MODE_SELECTOR_ID in config:
        sel = await cg.get_variable(config[CONF_MODE_SELECTOR_ID])
        cg.add(var.set_mode_selector(sel))

    # Arm strip
    if CONF_ARM_STRIP_ID in config:
        strip = await cg.get_variable(config[CONF_ARM_STRIP_ID])
        cg.add(var.set_arm_strip(strip))

    cg.add(var.set_force_prefix(config[CONF_FORCE_PREFIX]))
    cg.add(var.set_skip_delay_prefix(config[CONF_SKIP_DELAY_PREFIX]))

    await cg.register_component(var, config)
