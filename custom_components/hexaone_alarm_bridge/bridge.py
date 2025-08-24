"""Bridge logic for HexaOne Alarm Bridge."""
from homeassistant.core import HomeAssistant, ServiceCall
from homeassistant.const import ATTR_ENTITY_ID

from .const import DOMAIN, CONF_ALARMO_ENTITY


async def async_setup(hass: HomeAssistant, entry):
    """Register bridge services that forward calls to Alarmo."""
    alarmo_entity = entry.data[CONF_ALARMO_ENTITY]

    async def handle_arm(call: ServiceCall):
        data = {
            ATTR_ENTITY_ID: alarmo_entity,
            "code": call.data.get("code"),
            "mode": call.data["mode"],
            "skip_delay": call.data.get("skip_delay"),
            "force": call.data.get("force"),
        }
        await hass.services.async_call("alarmo", "arm", data)

    async def handle_disarm(call: ServiceCall):
        data = {
            ATTR_ENTITY_ID: alarmo_entity,
            "code": call.data.get("code"),
        }
        await hass.services.async_call("alarmo", "disarm", data)

    hass.services.async_register(DOMAIN, "arm", handle_arm)
    hass.services.async_register(DOMAIN, "disarm", handle_disarm)
