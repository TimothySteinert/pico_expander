import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select
from esphome.const import CONF_ID

from . import k1_ready_ns, K1Ready

K1ReadySelect = k1_ready_ns.class_("K1ReadySelect", select.Select, cg.Component)

CONF_K1_READY_ID = "k1_ready_id"

CONFIG_SCHEMA = select.select_schema(K1ReadySelect).extend(
    {
        cv.GenerateID(): cv.declare_id(K1ReadySelect),
        cv.Required(CONF_K1_READY_ID): cv.use_id(K1Ready),
    }
)

async def to_code(config):
    engine = await cg.get_variable(config[CONF_K1_READY_ID])
    var = cg.new_Pvariable(config[CONF_ID], engine)
    await cg.register_component(var, config)
    await select.register_select(var, config)
    # Bind so engine can push its current selection immediately
    cg.add(var.bind_to_engine())
