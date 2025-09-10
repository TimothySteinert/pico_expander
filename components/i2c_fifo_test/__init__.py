import esphome.codegen as cg
from esphome.components import i2c
import esphome.config_validation as cv
from esphome.const import CONF_ID

DEPENDENCIES = ["i2c"]
CODEOWNERS = ["@hexa-one"]
MULTI_CONF = False

i2c_fifo_test_ns = cg.esphome_ns.namespace("i2c_fifo_test")

I2CFifoTestComponent = i2c_fifo_test_ns.class_(
    "I2CFifoTestComponent", cg.Component, i2c.I2CDevice
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ID): cv.declare_id(I2CFifoTestComponent),
    }
).extend(i2c.i2c_device_schema(0x08))


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
