import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select, api
from esphome.const import CONF_ID

from . import k1_ready_ns

# Correct: use api.CustomAPIDevice (NOT cg.api.CustomAPIDevice)
K1ReadySelect = k1_ready_ns.class_("K1ReadySelect", select.Select, cg.Component, api.CustomAPIDevice)

CONF_OPTIONS = "options"
CONF_INITIAL_OPTION = "initial_option"
CONF_OPTIMISTIC = "optimistic"
CONF_RESTORE_VALUE = "restore_value"

CONFIG_SCHEMA = (
    select.select_schema(K1ReadySelect)
    .extend(
        {
            cv.Required(CONF_OPTIONS): cv.All(
                cv.ensure_list(cv.string),
                cv.Length(min=1),
            ),
            cv.Optional(CONF_INITIAL_OPTION): cv.string,
            cv.Optional(CONF_OPTIMISTIC, default=True): cv.boolean,
            cv.Optional(CONF_RESTORE_VALUE, default=True): cv.boolean,
        }
    )
    .add_extra_validation(
        lambda cfg: (
            cv.Schema({cv.Optional(CONF_INITIAL_OPTION): cv.In(cfg[CONF_OPTIONS])})(cfg)
            if CONF_INITIAL_OPTION in cfg
            else cfg
        )
    )
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])

    for opt in config[CONF_OPTIONS]:
        cg.add(var.add_option(opt))

    if CONF_INITIAL_OPTION in config:
        cg.add(var.set_initial_option(config[CONF_INITIAL_OPTION]))

    cg.add(var.set_optimistic(config[CONF_OPTIMISTIC]))
    cg.add(var.set_restore_value(config[CONF_RESTORE_VALUE]))

    await cg.register_component(var, config)
    await select.register_select(var, config)

    # Register the ready_update API service
    cg.add(var.register_ready_update_service())
