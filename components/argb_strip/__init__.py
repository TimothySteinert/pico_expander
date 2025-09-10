import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_PIN, CONF_NUM_LEDS
from esphome import pins

argb_strip_ns = cg.esphome_ns.namespace("argb_strip")
ARGBStripComponent = argb_strip_ns.class_("ARGBStripComponent", cg.Component)

CONF_GROUPS = "groups"
CONF_LEDS = "leds"
CONF_MAX_BRIGHTNESS = "max_brightness"
CONF_RFID_RAINBOW_CYCLE_MS = "rfid_rainbow_cycle_ms"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(ARGBStripComponent),
        cv.Required(CONF_PIN): pins.internal_gpio_output_pin_schema,
        cv.Required(CONF_NUM_LEDS): cv.positive_int,
        cv.Optional(CONF_RFID_RAINBOW_CYCLE_MS, default=8000): cv.int_range(min=500, max=60000),
        cv.Required(CONF_GROUPS): cv.Schema(
            {
                cv.string: cv.Schema(
                    {
                        cv.Required(CONF_LEDS): [cv.positive_int],
                        cv.Optional(CONF_MAX_BRIGHTNESS, default=255): cv.int_range(min=0, max=255),
                    }
                )
            }
        ),
    }
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    pin = await cg.gpio_pin_expression(config[CONF_PIN])
    cg.add(var.set_pin(pin))
    cg.add(var.set_raw_gpio(config[CONF_PIN]["number"]))
    cg.add(var.set_num_leds(config[CONF_NUM_LEDS]))
    cg.add(var.set_rfid_rainbow_cycle_ms(config[CONF_RFID_RAINBOW_CYCLE_MS]))

    for name, gconf in config[CONF_GROUPS].items():
        leds = gconf[CONF_LEDS]
        cap = gconf[CONF_MAX_BRIGHTNESS]
        cg.add(var.add_group(name, leds, cap))

    await cg.register_component(var, config)
