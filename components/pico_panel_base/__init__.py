import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.const import CONF_ID

DEPENDENCIES = ["i2c"]
MULTI_CONF = True
AUTO_LOAD = []

pico_ns = cg.esphome_ns.namespace("pico_panel_base")
PicoPanelBase = pico_ns.class_("PicoPanelBase", cg.Component, i2c.I2CDevice)

# optional knobs for early tests
CONF_RFID_KEY_ID = "rfid_key_id"
CONF_RFID_FLAGS  = "rfid_flags"
CONF_SYS_OPTIONS = "sys_options"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(PicoPanelBase),
            cv.Optional(CONF_RFID_KEY_ID, default=0x10): cv.int_range(min=0, max=255),
            cv.Optional(CONF_RFID_FLAGS,  default=0xA0): cv.int_range(min=0, max=255),
            cv.Optional(CONF_SYS_OPTIONS, default=0x55): cv.int_range(min=0, max=255),
        }
    )
    .extend(i2c.i2c_device_schema(0x30))  # default addr 0x30
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)  # wires IDF/Arduino bus automatically :contentReference[oaicite:4]{index=4}
    cg.add(var.set_rfid_key_id(config[CONF_RFID_KEY_ID]))
    cg.add(var.set_rfid_flags(config[CONF_RFID_FLAGS]))
    cg.add(var.set_sys_options(config[CONF_SYS_OPTIONS]))
