# Changelog

## 0.6.1 — 2026-10-14

### Fixed

- Apply the configured Locale to GeoPosition and pressure-temperature age inputs,
  restore previews and reduction-age diagnostics. German uses decimal commas;
  English uses decimal points. Keep storage, backups and MQTT locale-neutral.
- Document GeoPosition and sea-level pressure in the German firmware guide.

## 0.6.0 — 2026-10-14

### Added

- Optional latitude, longitude and actual sensor altitude, with zero altitude
  distinguished from missing configuration.
- A separate derived sea-level pressure channel for physical and simulated
  barometers, available through Properties, display selection, MQTT and
  Home Assistant discovery while retaining local atmospheric pressure.
- Selectable local outdoor-temperature input with configurable freshness
  checking, temperature-aware reduction and standard-atmosphere fallback.
- Reduction diagnostics identifying method, temperature source, sample age
  and fallback or quality reason.

### Changed

- Persist location and temperature-source settings in NVS and include them
  in the Web configuration and backup/restore preview. Backup/snapshot
  format 3 retains compatibility with versions 1 and 2.
- Keep derivation separate from measurement quality: fresh, good temperature
  preserves pressure quality; missing or restricted temperature estimates
  good pressure without upgrading restricted pressure quality.

### Validation

- 398 native tests passed, ESP32 firmware built successfully, and the new
  functionality was verified on the device.

## 0.5.1 — 2026-10-13

### Fixed

- Keep the Device page's Hardware and Provisioning button below the hardware
  details with consistent spacing, preventing overlap with the serial number.
- Correct the timezone documentation link on the published setup page.
