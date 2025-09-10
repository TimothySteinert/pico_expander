import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.const import CONF_ID, CONF_PIN

CONF_NUM_LEDS = "num_leds"
CONF_GROUPS = "groups"
CONF_GLOBAL_BRIGHTNESS = "global_brightness"
CONF_GAMMA = "gamma"

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
        cv.Optional(CONF_GLOBAL_BRIGHTNESS, default=1.0): cv.float_range(min=0.0, max=1.0),
        cv.Optional(CONF_GAMMA, default=False): cv.boolean,
    }
).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    pin_expr = await cg.gpio_pin_expression(config[CONF_PIN])
    cg.add(var.set_pin(pin_expr))
    cg.add(var.set_num_leds(config[CONF_NUM_LEDS]))
    cg.add(var.set_raw_gpio(config[CONF_PIN]['number']))
    cg.add(var.set_global_brightness(config[CONF_GLOBAL_BRIGHTNESS]))
    cg.add(var.set_gamma_enabled(config[CONF_GAMMA]))

    num_leds = config[CONF_NUM_LEDS]
    for name, led_list in config[CONF_GROUPS].items():
        for led in led_list:
            if led > num_leds:
                raise cv.Invalid(f"Group '{name}' references LED {led}, but num_leds is {num_leds}")
        zero_based = [l - 1 for l in led_list]
        cg.add(var.add_group(cg.std_string(name), zero_based))
