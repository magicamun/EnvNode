# ADR-0003

# Time Handling

- Status: Accepted
- Date: 2026-08-07

---

## Context

Many weather measurements require an accurate timestamp.

Internally, the firmware performs scheduling, timeout handling and runtime calculations.

Externally, Measurements are published to MQTT and consumed by Home Assistant and other automation systems.

Different representations of time are suitable for different purposes.

---

## Decision

WeatherStation uses two different time representations.

Internally:

- Unix Epoch (`time_t`)

Externally:

- ISO-8601 local time including UTC offset

Examples:

UTC

    2026-08-07T03:34:50Z

Local

    2026-08-07T05:34:50+02:00

The configured timezone determines the local representation.

Daylight-saving transitions are handled automatically by the timezone definition.

---

## Rationale

Epoch time provides:

- efficient storage
- efficient comparisons
- unambiguous calculations
- independence from local timezones

Local ISO-8601 timestamps provide:

- human readability
- interoperability
- explicit timezone information
- correct historical interpretation

Using local timestamps without UTC offset is intentionally avoided.

---

## Consequences

Infrastructure services use Unix Epoch internally.

Published timestamps use local ISO-8601 including UTC offset.

MQTT connectivity may exist before time synchronization.

Measurement publication should begin only after valid synchronized system time is available.

---

## Alternatives Considered

### UTC everywhere

Advantages

- simple implementation

Disadvantages

- poor readability
- inconvenient for users
- external software must always perform timezone conversion

---

### Local time without UTC offset

Rejected.

Ambiguous timestamps during daylight-saving transitions make this unsuitable for published Measurements.