# HexaOne Alarm Bridge

This custom integration bridges **HexaOne ESPHome keypads** with the **Alarmo** integration in Home Assistant.

## Features
- Exposes services to arm and disarm Alarmo via HexaOne keypad input.
- Supports all Alarmo modes:
  - `away`, `home`, `night`, `vacation`, `custom`
- Forwards options directly to Alarmo:
  - `code` (forwarded unchanged, Alarmo validates it)
  - `skip_delay` (skip exit delay)
  - `force` (bypass open sensors)
- Configurable via UI (config flow) → choose the target Alarmo entity.

## Installation
1. Copy this folder into `custom_components/hexaone_alarm_bridge/`.
2. Restart Home Assistant.
3. Add the integration through **Settings → Devices & Services → Add Integration → HexaOne Alarm Bridge**.
4. Select the Alarmo entity you want to control.

## Services
- `hexaone_alarm_bridge.arm`
- `hexaone_alarm_bridge.disarm`

See `services.yaml` for full schema.

## Future
This bridge will later be combined with the reverse bridge (Alarmo → ESPHome keypad) into a single **HexaOne Alarm** integration.
