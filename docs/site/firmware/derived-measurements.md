<!-- SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0 -->
<!-- See the repository root LICENSE. -->
# Derived measurements

EnvNode exposes derived physical quantities as separate measurement channels.
Their MQTT payloads contain `derived: true`. Raw channels remain available.
These calculations do not perform forecasting, aggregation or automation.

## Dew point and absolute humidity

AM2302, BME280, SHT4x and SHTC3 instances expose `dew_point` (°C, or the
configured temperature presentation unit) and `absolute_humidity` (g/m³).
They share the originating sensor's ID, acquisition schedule and timestamp.
No extra slot or cross-sensor source selection is required.

The `HumidityDerivedSensor` decorator pairs temperature and relative humidity
from the same wrapped instance and the same `sample()` or `service()` call.
It never fills a missing input from the snapshot cache or another instance.
A partial acquisition produces invalid derived values; no acquisition produces
no additional messages. Separate simulated temperature and humidity instances
therefore do not produce these channels.

For temperature T in °C and relative humidity H in percent:

- γ = ln(H / 100) + 17.62 T / (243.12 + T)
- dew point = 243.12 γ / (17.62 − γ)
- vapour pressure e = (H / 100) × 6.112 × exp(17.62 T / (243.12 + T)), in hPa
- absolute humidity = 216.7 e / (273.15 + T), in g/m³

These are Magnus approximations over liquid water (including supercooled water),
not frost-point calculations. Inputs are restricted to −45…60 °C and 0…100 % RH.
At 0 % RH, absolute humidity is zero and dew point is invalid, rather than
publishing an infinite number. Missing, invalid or out-of-range inputs produce
`valid: false` without `value`. Valid results have `good` quality when both inputs are good;
otherwise they preserve the worse input quality (`estimated`,
`below_measurement_range`, or `degraded`). Derivation alone does not reduce quality.

References: [Sensirion, Humidity at a glance](https://sensirion.com/resource/application_note/sht/glance),
[Vaisala, dew point calculation](https://docs.vaisala.com/r/M212417EN-L/en-US/GUID-EBE2B115-3C98-4C8C-9F2D-A2FF1EFECFCC).

## Solar position

Choose **Solar Position** (`solar_position`) in an unused sensor slot. It needs
no GPIO or I²C resource. Its default interval is 60 seconds and can be configured
like other periodic sensors. It exposes:

- `solar_azimuth`, degrees clockwise from true north: north 0°, east 90°,
  south 180°, west 270°; range [0, 360).
- `solar_elevation`, degrees above the geometric horizon; negative at night.

Both use configured latitude/longitude and synchronized UTC, independently of
the configured display timezone and daylight saving time. Altitude is not
required. The source is calculated, not simulated. Results are marked
`derived: true`, `simulated: false` and `quality: good` when the location and
synchronized UTC are valid. Invalid prerequisites produce `quality: degraded`.

The solar-centre calculation uses the
[USNO approximate solar coordinates](https://aa.usno.navy.mil/faq/sun_approx)
and the [horizontal coordinate transformation](https://aa.usno.navy.mil/faq/alt_az).
It has no atmospheric refraction, terrain, horizon dip or observer-height
correction. The implementation accepts years 1800–2199 where representable by
platform `time_t`. A missing/invalid location or undefined zenith/nadir azimuth
makes the pair invalid. Before time synchronization the SensorManager discards
all measurements as for physical sensors. A valid location and time allow
recovery without disabling the source.

## Integration and limits

All channels use the existing SensorManager → snapshot cache → MQTT pipeline.
Home Assistant Discovery includes the new channels and maps invalid derived
values to an unavailable numeric state through the value template. Properties,
display sources and threshold source selection use their ordinary stable keys.
Dew point shares the configured temperature presentation unit.

Sensor implementation metadata now permits six channels (BME280 uses all six).
Discovery uses 32-bit masks, with a compile-time check against the number of
measurement types. Existing measurement IDs and unit IDs remain unchanged;
the four new measurement types and two new units are appended.
