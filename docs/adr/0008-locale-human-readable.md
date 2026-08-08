# ADR-0008

# Locale and Human-Readable Formatting

- Status: Accepted
- Date: 2026-08-07

---

## Context

WeatherStation internally uses stable machine-oriented representations.

Examples include:

- Unix Epoch timestamps
- POSIX timezone rules
- canonical Measurement units
- numeric values independent from regional display conventions

External users, however, expect human-readable representation according to regional conventions.

Examples include:

- date order
- 12-hour versus 24-hour time
- decimal separators
- thousands separators
- future translated UI labels

Without an explicit formatting boundary, locale-specific representation could become mixed into:

- TimeService
- Sensor implementations
- MeasurementPublisher
- WebService
- individual pages

That would couple domain and infrastructure logic to presentation preferences.

WeatherStation therefore requires a dedicated Locale concept for human-readable formatting.

---

## Decision

WeatherStation separates:

- Timezone
- Locale
- Presentation Unit

These are independent configuration dimensions.

Conceptually:

    Unix Epoch
        |
        v
    TimeService
        |
        | timezone application
        v
    local absolute time
        |
        v
    Locale Formatter
        |
        | regional presentation
        v
    human-readable output

Measurement units follow ADR-0005 independently:

    canonical Measurement
        |
        v
    Presentation Unit
        |
        v
    external numeric representation

Locale affects formatting.

Presentation Unit affects physical-unit representation.

Timezone affects local civil time.

These responsibilities must not be merged.

---

## Initial Locales

The initial implementation supports:

- `de-DE` — German (Germany)
- `en-GB` — English (United Kingdom)
- `en-US` — English (United States)

Additional locales may be added later without changing the architecture.

The initial locale set is intentionally small.

The purpose is architectural preparation rather than comprehensive internationalization.

---

## Locale Configuration

Locale is persistent Device configuration.

Conceptually:

    LocaleConfiguration
        |
        +---- locale

Locale is represented by a typed identifier rather than arbitrary free-form text.

Examples:

    de-DE
    en-GB
    en-US

ConfigurationService remains the authoritative owner of Locale configuration.

Locale configuration is global for the Device.

---

## Timezone and Locale

Timezone and Locale are independent.

Timezone answers:

> Which local civil time applies?

Locale answers:

> How should that time be presented to a human?

Example:

    Timezone:
        Europe/Berlin

    Locale:
        en-US

is valid.

Likewise:

    Timezone:
        America/New_York

    Locale:
        de-DE

is technically valid.

The Web Interface may present sensible defaults but must not force one configuration from the other.

---

## Date and Time Formatting

Human-readable timestamps are formatted according to Locale.

Initial examples:

### de-DE

    07.08.2026 20:15:30

### en-GB

    07/08/2026 20:15:30

### en-US

    08/07/2026 08:15:30 PM

The exact formatting implementation may evolve, but the responsibility boundary remains stable.

Machine-facing timestamps may continue to use ISO-8601 where required.

Human-readable Web Interface output uses Locale formatting.

Locale formatting must never alter the underlying Unix Epoch timestamp.

---

## Number Formatting

Locale may define regional numeric presentation rules.

Examples include:

### de-DE

    20,5
    1.013,2

### en-GB / en-US

    20.5
    1,013.2

Number formatting is presentation only.

Canonical numeric values remain unchanged internally.

The initial implementation may support only the formatting required by the current Web Interface.

More advanced localization may be added later.

---

## Relationship to Presentation Units

Locale does not determine Presentation Units.

ADR-0005 remains authoritative for physical-unit selection.

Examples:

    Locale:
        en-US

    Temperature Presentation Unit:
        °C

is valid.

and:

    Locale:
        de-DE

    Temperature Presentation Unit:
        °F

is also valid.

The system must never silently change configured units when Locale changes.

Locale and Presentation Unit configuration may have regional defaults in the future, but explicit user configuration always remains independent.

---

## Locale Formatting Boundary

Locale-specific formatting belongs to a dedicated formatting abstraction.

Conceptually:

    LocaleFormatter
        |
        +---- formatDateTime(...)
        +---- formatDate(...)
        +---- formatTime(...)
        +---- formatNumber(...)

The exact class name is an implementation detail.

The formatting abstraction consumes already normalized values.

It does not own:

- time synchronization
- timezone calculation
- Sensor acquisition
- Measurement conversion
- unit conversion
- configuration persistence

---

## TimeService Responsibility

TimeService remains responsible for:

- Unix Epoch system time
- SNTP synchronization
- timezone configuration
- local civil-time conversion
- machine-oriented time representation

TimeService does not become responsible for:

- regional date formatting
- 12-hour versus 24-hour display preference
- decimal separators
- UI translation

This preserves the separation between time infrastructure and human presentation.

---

## Web Administration

The existing Time administration area becomes conceptually:

    Locale & Time

It contains two separate configuration groups.

### Locale

- German (Germany)
- English (United Kingdom)
- English (United States)

### Time

- timezone
- NTP server 1
- NTP server 2

Runtime synchronization state remains part of Time diagnostics.

Locale affects only human-readable presentation.

---

## UI Language

The initial implementation does not require full Web Interface translation.

UI text may remain English.

The locale architecture must nevertheless allow future translation without redesigning:

- configuration
- routing
- subsystem interfaces
- Measurement handling

Future UI localization may introduce typed text resources or translation tables.

That work is intentionally deferred.

---

## Progressive Localization

Localization is introduced incrementally.

Initial scope:

- typed Locale configuration
- regional date formatting
- regional time formatting
- basic number formatting where useful
- `de-DE`
- `en-GB`
- `en-US`

Deferred scope:

- translated UI labels
- pluralization rules
- comprehensive number/date formatting libraries
- arbitrary locale installation
- browser-language auto-negotiation

The architecture must support these future capabilities without requiring them now.

---

## Consequences

TimeService remains focused on time infrastructure.

WebService no longer needs to contain ad-hoc date-formatting rules.

Locale-specific presentation becomes reusable across pages.

Changing Locale does not affect:

- system time
- timezone
- Sensor acquisition
- canonical Measurements
- Presentation Unit configuration
- MQTT transport

Locale changes affect future human-readable output only.

---

## Alternatives Considered

### Use ISO-8601 everywhere

Rejected as the only human-facing representation.

ISO-8601 remains useful for machine-facing communication but does not satisfy regional presentation expectations for the Web Interface.

---

### Let each Web page format dates independently

Rejected.

This duplicates formatting rules and leads to inconsistent representation.

---

### Put Locale handling inside TimeService

Rejected.

TimeService owns time semantics and timezone handling.

Regional display formatting is a separate responsibility.

---

### Derive Presentation Units automatically from Locale

Rejected.

Physical-unit preference and regional formatting are independent concerns.

Users may intentionally select units that differ from common regional defaults.

---

### Fully internationalize the UI immediately

Rejected.

Comprehensive localization is unnecessary at the current project stage.

The architecture is prepared now while implementation remains intentionally small.

---

## Design Rule

Time defines when.

Timezone defines where in civil time.

Locale defines how humans see it.

Presentation Units define how physical quantities are represented.

None of these responsibilities silently controls another.