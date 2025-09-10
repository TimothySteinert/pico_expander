import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.const import CONF_ID, CONF_PIN

CONF_NUM_LEDS = "num_leds"
CONF_GROUPS = "groups"

argb_strip_ns = cg.esphome_ns.namespace("argb_strip")
ARGBStripComponent = argb_strip_ns.class_("ARGBStripComponent", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(ARGBStripComponent),
        cv.Required(CONF_PIN): pins.internal_gpio_output_pin_schema,
        cv.Required(CONF_NUM_LEDS): cv.int_range(min=1, max=2048),
        cv.Required(CONF_GROUPS): cv.Schema({
            cv.valid_name: cv.All(
                cv.ensure_list(cv.int_range(min=1)),
                cv.Length(min=1)
            )
        }),
    }
).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    # Standard ESPHome pin expression (still used for high-level ops, direction, etc.)
    pin_expr = await cg.gpio_pin_expression(config[CONF_PIN])
    cg.add(var.set_pin(pin_expr))

    num_leds = config[CONF_NUM_LEDS]
    cg.add(var.set_num_leds(num_leds))

    # Extract raw pin number directly from the YAML config
    # The internal_gpio_output_pin_schema stores it under key 'number'
    raw_pin_num = config[CONF_PIN]['number']
    cg.add(var.set_raw_gpio(raw_pin_num))

    for name, led_list in config[CONF_GROUPS].items():
        for led in led_list:
            if led > num_leds:
                raise cv.Invalid(f"Group '{name}' references LED {led}, but num_leds is {num_leds}")
        zero_based = [l - 1 for l in led_list]
        cg.add(var.add_group(cg.std_string(name), zero_based))
