# Derived sea-level pressure

License: LicenseRef-EnvNode-Use-and-Redistribution-1.0; see the root LICENSE.

EnvNode preserves the locally measured atmospheric pressure and optionally provides
an explicitly derived sea-level pressure. Configure actual sensor altitude above sea
level under Device settings, then apply the Sensor changes or restart. A sensor on a
building uses the altitude of the sensor, including building height. A blank altitude
is absent; 0 m is valid. Latitude and longitude can be stored independently and are
not used by this calculation.

Without a selected, usable local temperature the reduction uses the standard atmosphere:

`pNN = p / (1 - h / 44330)^5.255`

Here `p` and `pNN` are Pascal and `h` is metres. Good input produces an estimated
value; degraded input quality remains degraded. Missing height, invalid local
pressure, non-finite/non-positive pressure or unsupported height produces an invalid
derived value. Other channels continue normally. The permitted installation altitude
range is -1000 to 10000 m; the approximation does not account for actual atmospheric
temperature profiles.

For the supplied example, 950.3 hPa at 694 m gives approximately 1032.5 hPa. The
older station's 1031.03 hPa and temperature-dependent approximation of about
1031.07 hPa at 15.2 °C are plausibility comparisons, not calibration targets. No
fixed temperature is stored. An enabled local temperature Sensor can now be selected
with a configurable maximum age (default 60 seconds).

The `sea_level_pressure` property is available in the display editor, measurement
view, MQTT and Home Assistant discovery. MQTT and the measurement view use the same selectable pressure units
as local atmospheric pressure. OLED property formatting uses canonical Pascal,
as for the existing local-pressure property. Display layouts are user-configurable; choose the new
property explicitly. The original local pressure and existing layouts are retained.
Version-1 and version-2 backups/NVS snapshots remain readable. Version 1 represents
unknown location; both versions default to no selected temperature.

## Quality policy

Derivation and quality are separate: `derived: true` describes the origin of the
sea-level value and is not a quality level. The standard-atmosphere method
uses no measured temperature, so good local pressure produces `estimated` quality.

For the temperature-aware method, a valid, fresh outdoor-temperature
measurement with `good` quality allows the derived value to retain the local
pressure measurement's quality. If no temperature is used, or its quality is not
`good`, the reduction is `estimated` when the local pressure is good. An already
restricted local-pressure quality must never be upgraded; `degraded` or
`below_measurement_range` remains visible. Missing, invalid or stale temperature
causes fallback to the standard-atmosphere formula.

## Local temperature source

Under Device settings, select a local outdoor-temperature source by Slot and name.
Only enabled Sensor slots with an enabled schedule and the `temperature` channel
are offered. Simulated sources are marked explicitly. Solar-cell temperature and
external MQTT values are not eligible. The selection is device-wide for all local
barometers. Clear the selection before disabling or replacing its source with an
incompatible implementation; configuration validation protects this reference.

Save the source and maximum age, then apply Sensor changes or restart. The latest
accepted temperature is used whenever a pressure measurement arrives. Freshness
uses monotonic elapsed time, including counter wrap; the age limit is inclusive.
Until a selected source has provided its first valid sample, or after its sample
becomes stale, standard-atmosphere reduction continues. Sensor ordering can make
the first pressure use fallback until the next pressure sample. For combined
Sensors, a temperature emitted before pressure in the same operation is already
available through the shared cache. Invalid temperature updates replace previously
valid ones and cause fallback immediately on the next pressure measurement.

The temperature-aware dry-air approximation is
`pNN = p * (1 + 0.0065*h/(T + 273.15))^(9.80665/(287.05*0.0065))`, with station
outdoor temperature `T` in Celsius. This follows the constant-lapse-rate form of
the hydrostatic/hypsometric relation described by
[NOAA MADIS](https://madis.ncep.noaa.gov/madis_rwis_qc_notes.shtml).
It does not implement the full MAPS-SLP algorithm, humidity correction or measured
vertical temperature profiles. For 950.3 hPa at 694 m and 15.2 °C it gives about
1031.08 hPa. The comparison remains a plausibility check, not calibration.

Fresh, valid temperature with restricted quality is still used, while good local
pressure becomes `estimated`. Restricted pressure quality is retained. Invalid or
non-finite values, temperatures at/below absolute zero, or an unusable extrapolated
column temperature trigger fallback. A simulated input used by the calculation
marks the derived value as simulated; local pressure provenance remains unchanged.

The measurement view shows the method, source, age and reason. MQTT adds
`derivation` (`local_temperature`, `standard_atmosphere` or `none`),
`reduction_reason`, `temperature_source` and, when available, `temperature_age_ms`.
Diagnostics describe the last pressure reduction attempt, not a continuously updated
age. Display and Home Assistant retain the same sea-level-pressure channel.

## Locale at the Web boundary

From firmware 0.6.1, location and temperature-age fields use the configured Locale:
German decimal commas, English decimal points. Editable numbers have no thousands
separators. Web adapters normalize these strings before invoking the neutral domain
parsers. Restore previews and reduction-age diagnostics use the same Locale.
MQTT, NVS and backup/snapshot representations remain unchanged and locale-neutral.
Firmware 0.6.0 location inputs still require the neutral decimal-point representation.
