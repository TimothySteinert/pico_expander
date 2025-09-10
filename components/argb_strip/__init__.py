import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.const import CONF_ID, CONF_PIN

CONF_NUM_LEDS = "num_leds"
CONF_GROUPS = "groups"
CONF_LEDS = "leds"
CONF_MAX_BRIGHTNESS = "max_brightness"

argb_strip_ns = cg.esphome_ns.namespace("argb_strip")
ARGBStripComponent = argb_strip_ns.class_("ARGBStripComponent", cg.Component)

def _validate_group_block(value):
    if isinstance(value, list):
        return {CONF_LEDS: value, CONF_MAX_BRIGHTNESS: 1.0}
    if isinstance(value, dict):
        if CONF_LEDS not in value:
            raise cv.Invalid(f"Group dict must contain '{CONF_LEDS}' key")
        leds = cv.ensure_list(cv.int_range(min=1))(value[CONF_LEDS])
        mb = value.get(CONF_MAX_BRIGHTNESS, 1.0)
        mb = cv.float_range(min=0.0, max=1.0)(mb)
        return {CONF_LEDS: leds, CONF_MAX_BRIGHTNESS: mb}
    raise cv.Invalid("Group value must be a list of LED numbers or a dict with 'leds' and optional 'max_brightness'")

def groups_schema(value):
    if not isinstance(value, dict):
        raise cv.Invalid("groups must be a mapping")
    out = {}
    for name, v in value.items():
        name_v = cv.valid_name(name)
        out[name_v] = _validate_group_block(v)
    return out

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(ARGBStripComponent),
        cv.Required(CONF_PIN): pins.internal_gpio_output_pin_schema,
        cv.Required(CONF_NUM_LEDS): cv.int_range(min=1, max=2048),
        cv.Required(CONF_GROUPS): groups_schema,
    }
).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    pin_expr = await cg.gpio_pin_expression(config[CONF_PIN])
    cg.add(var.set_pin(pin_expr))
    num_leds = config[CONF_NUM_LEDS]
    cg.add(var.set_num_leds(num_leds))
    cg.add(var.set_raw_gpio(config[CONF_PIN]['number']))

    for name, grp in config[CONF_GROUPS].items():
        leds = grp[CONF_LEDS]
        max_brt = grp[CONF_MAX_BRIGHTNESS]
        for led in leds:
            if led > num_leds:
                raise cv.Invalid(f"Group '{name}' references LED {led} beyond num_leds={num_leds}")
        zero_based = [l - 1 for l in leds]
        cg.add(var.add_group(cg.std_string(name), zero_based, max_brt))
