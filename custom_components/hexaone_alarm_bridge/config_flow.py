"""Config flow for HexaOne Alarm Bridge."""
from __future__ import annotations

import voluptuous as vol

from homeassistant import config_entries
from homeassistant.const import CONF_ENTITY_ID
from homeassistant.helpers import entity_registry as er

from .const import DOMAIN, CONF_ALARMO_ENTITY


class HexaOneAlarmBridgeFlow(config_entries.ConfigFlow, domain=DOMAIN):
    """Handle a config flow for HexaOne Alarm Bridge."""

    VERSION = 1

    async def async_step_user(self, user_input=None):
        errors = {}
        if user_input is not None:
            return self.async_create_entry(
                title="HexaOne Alarm Bridge",
                data={CONF_ALARMO_ENTITY: user_input[CONF_ENTITY_ID]},
            )

        entity_registry = er.async_get(self.hass)
        alarmo_entities = [
            e.entity_id
            for e in entity_registry.entities.values()
            if e.domain == "alarm_control_panel" and e.platform == "alarmo"
        ]

        schema = vol.Schema(
            {vol.Required(CONF_ENTITY_ID): vol.In(alarmo_entities)}
        )

        return self.async_show_form(step_id="user", data_schema=schema, errors=errors)
