# ADR-0001: Measure, Don't Interpret

## Status

Accepted

## Context

Traditional weather stations often combine sensor acquisition, aggregation, statistics and interpretation inside the device firmware.

This creates several problems:

- algorithms are difficult to inspect or replace
- firmware becomes increasingly complex
- calculation errors can invalidate otherwise correct measurements
- historical interpretation cannot easily be recalculated
- the device becomes tightly coupled to one use case

WeatherStation is intended to serve multiple automation and analysis systems.

Therefore the boundary between measurement and interpretation must be explicit.

## Decision

WeatherStation firmware shall primarily publish raw or hardware-near physical measurements.

Higher-level interpretation and aggregation shall be performed outside the device.

Examples of values that belong inside the firmware:

- temperature
- relative humidity
- station pressure
- solar irradiance
- rain detector state
- rain detector raw measurement
- rain gauge tip events
- sensor temperature
- hardware heater output

Examples of values that belong outside the firmware:

- rainfall today
- rainfall yesterday
- rainfall per hour
- sunshine duration
- pressure trend
- daily solar energy
- evapotranspiration
- irrigation decisions

Hardware-related conversion and compensation remain part of the firmware when required to obtain a meaningful physical measurement.

## Consequences

### Positive

- simpler firmware
- easier testing
- easier sensor replacement
- transparent algorithms
- historical values can be recalculated
- fewer dependencies between measurement and application logic
- easier reuse outside Home Assistant

### Negative

- an external system is required for aggregation and interpretation
- some values commonly provided by weather stations are not directly available from the device
- downstream software must implement its own persistence and statistics

These consequences are accepted because they preserve reliable raw measurements and keep the embedded device maintainable.