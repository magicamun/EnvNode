# ADR-0007

# Web Administration Architecture

- Status: Accepted
- Date: 2026-08-07

---

## Context

WeatherStation is intended to operate unattended after deployment.

The embedded Web Interface provides local administration without requiring external systems such as Home Assistant.

As the firmware grows, administration expands beyond initial provisioning and now includes:

- Device configuration
- Sensor configuration
- Presentation Unit configuration
- Diagnostics
- Runtime status
- Firmware management
- Factory reset

A single configuration page is no longer sufficient.

The administration interface must reflect the architectural separation already established in:

- ADR-0004 (Measurement Pipeline)
- ADR-0005 (Canonical and Presentation Units)
- ADR-0006 (Device Configuration Model)

The Web Interface therefore becomes an administration client rather than a configuration owner.

---

## Decision

The embedded Web Interface is organized into functional administration sections.

Conceptually:

    Home
        |
        +---- Status
        |
        +---- Sensors
        |
        +---- Network
        |
        +---- MQTT
        |
        +---- Time
        |
        +---- Units
        |
        +---- Device
        |
        +---- Diagnostics
        |
        +---- Logs
        |
        +---- Firmware

The exact visual representation may evolve.

The functional separation remains stable.

---

## Responsibility

The Web Interface owns:

- displaying configuration
- editing configuration
- displaying runtime state
- displaying diagnostics
- invoking supported administrative actions

The Web Interface does not own:

- configuration persistence
- sensor creation
- measurement acquisition
- MQTT communication
- time synchronization
- hardware interaction

Every configuration change is delegated to ConfigurationService.

## Implementation status (2026-08-17)

Implemented and extended with Sensors, Actuators and Controllers administration. WebService remains an adapter:

- configuration edits use `IConfigurationService` and explicit runtime-apply actions
- Actuator runtime commands use `ActuatorRuntime` capability lookup
- Controller Start/Stop uses `ControllerRuntime`
- Blink and Threshold edits use typed Controller Slot configuration
- Threshold source Measurements are derived from the selected Sensor implementation's metadata
- target filtering reflects `OnOff` capability and other enabled Controllers' ownership
- Logs provides read-only access to canonical entries retained in the 64-entry RAM log store

The `GET /logs` page receives a narrow `IRecentLogReader` dependency. It copies entries
without exposing mutable storage, displays newest first, and shares timestamp formatting
with the Serial renderer. It has manual refresh only: no clear action, streaming, polling,
JSON API, or persistence. Framework/ESP-IDF UART output is outside this view because it
does not pass through the EnvNode structured logger.

ConfigurationService remains authoritative over compatibility, cross-reference integrity and exclusive Controller target ownership. WebService does not construct domain objects, manipulate GPIO, or call concrete Actuator or Controller implementations.

---

## Relationship to Configuration

The Web Interface is not the authoritative configuration source.

Conceptually:

        Browser
            |
            v
      Web Interface
            |
            v
    ConfigurationService
            |
            v
      Configuration

ConfigurationService performs:

- validation
- persistence
- runtime update
- default handling

The Web Interface only presents and edits configuration.

---

## Relationship to Runtime State

Configuration and runtime state remain separate.

Examples:

Configuration:

    Hostname = WeatherStation

Runtime:

    Connected
    Current IP = 192.168.20.200

Configuration:

    Sensor enabled = true

Runtime:

    Sensor state = Ready

Configuration:

    Temperature presentation = °F

Runtime:

    Last published value = 74.3 °F

The Web Interface may display both simultaneously.

They remain different architectural concepts.

---

## Status Page

Status provides a concise operational overview.

Typical information includes:

- firmware version
- uptime
- hostname
- current IP address
- WiFi status
- MQTT status
- time synchronization
- heap usage
- restart reason

Status is read-only.

---

## Sensor Administration

Sensor administration is based on logical Sensor Slots.

A Sensor Slot represents one logical measurement source.

It is independent from the concrete Sensor implementation.

Each Slot contains configuration such as:

- SensorId
- enabled
- implementation
- acquisition schedule

and displays runtime information such as:

- detected
- provenance
- operational state
- last Measurement

Configured implementation, physical detection and operational state remain separate.

Example:

Configured:

    BMP390

Detected:

    No

State:

    Failed

This is a valid runtime situation.

The Web Interface must never silently modify configuration based on hardware detection.

---

## Network Administration

Network administration manages:

- WiFi credentials
- hostname
- address mode

Address mode supports:

- DHCP
- Static IPv4

Static configuration includes:

- IP address
- subnet mask
- gateway
- DNS server(s)

Runtime network status is displayed separately from configured values.

---

## MQTT Administration

MQTT administration manages:

- broker
- port
- authentication

Future MQTT-specific settings may be added without affecting Measurement semantics.

---

## Time Administration

Time administration manages:

- timezone
- NTP servers

Runtime synchronization state is displayed separately.

Examples include:

- synchronized
- synchronizing
- last synchronization

---

## Presentation Units

Presentation Unit configuration follows ADR-0005.

Units are configured globally per MeasurementType.

Example:

Temperature

    Celsius

Pressure

    Hectopascal

Humidity

    Percent

Changing Presentation Units affects future published representations only.

---

## Device Administration

General Device settings include:

- device name
- future installation metadata
- future operating options

Device administration intentionally excludes subsystem-specific settings.

---

## Diagnostics

Diagnostics present operational information collected by subsystems.

Examples include:

WiFi

MQTT

Time

SensorManager

Sensors

System

Diagnostics are read-only.

Diagnostics never become configuration.

---

## Firmware

Firmware administration provides:

- firmware version
- firmware upload
- OTA update
- restart
- factory reset

Firmware management remains independent from Sensor configuration.

---

## Restart Requirement

Configuration changes belong to one of two categories.

Runtime-applicable

Changes become active immediately.

Restart-required

Changes become active after Device restart.

Examples of restart-required settings include:

- hostname
- WiFi credentials
- DHCP / Static selection
- IP configuration

The Web Interface must clearly indicate when a restart is required.

Restart requirements belong to configuration metadata rather than individual pages.

---

## Administrative Actions

Typical administrative actions include:

- Restart
- Factory Reset
- Firmware Update

Administrative actions are explicit.

The Web Interface never performs implicit configuration changes.

---

## Separation of Concerns

The Web Interface communicates only through public subsystem interfaces.

Examples:

ConfigurationService

TimeService

SensorManager

Diagnostics

The Web Interface never manipulates subsystem internals directly.

---

## Consequences

The administration interface scales naturally as WeatherStation gains functionality.

Adding a new subsystem typically requires:

- one navigation entry
- one configuration editor where applicable
- one runtime view where applicable

Existing administration pages remain unaffected.

Configuration ownership remains centralized.

Subsystem responsibilities remain unchanged.

---

## Alternatives Considered

### One single configuration page

Rejected.

The number of independent configuration areas has outgrown a flat administration model.

---

### One web page per implementation class

Rejected.

Administration follows functional responsibilities rather than implementation details.

---

### Let the Web Interface own configuration

Rejected.

ConfigurationService remains the authoritative owner of configuration.

---

### Merge runtime state into configuration

Rejected.

Configuration and runtime state describe different concepts and evolve independently.

---

## Design Rule

The Web Interface administers.

ConfigurationService owns configuration.

Subsystems own runtime behaviour.

Diagnostics describe reality.

The administration interface reflects architecture rather than defining it.

Progressive Disclosure
The administration interface presents functionality appropriate to the selected administration level. Common operational tasks remain simple, while advanced configuration and developer-oriented diagnostics are revealed only when explicitly enabled. The administration level affects presentation only. It never changes subsystem behaviour, configuration ownership or runtime semantics.

## Fixed-length response delivery — 2026-11-04

HTML pages presented through WebHost and the shared stylesheet use a cooperative
response sender rather than WiFiClient's synchronous write retry loop. The host
retains the socket and rendered page after the route handler returns. It sends at
most 2 KiB in up to four nonblocking calls per application loop, using chunks of
at most 1 KiB. Partial writes advance only by the returned byte count; EAGAIN,
EWOULDBLOCK and EINTR retain the current offset for the next loop. Measurements
and integrations continue while a client is slow. One response is active at a
time, and the host accepts the next request after it completes or aborts.

The SDK still generates HTTP status/headers. The host captures those headers
(up to 2 KiB) before sending any bytes and preserves the declared Content-Length.
The page header/body are moved into the response owner where possible; static
footer/CSS data remain borrowed. There is no extra serialized copy of the complete
page and no per-step allocation. Total retained response size is limited to
64 KiB. Failure to capture/retain a response aborts the connection with an explicit
log; it does not silently continue a partially generated page.

An idle client times out after 10 seconds without send progress, with a 60-second
total limit. Completion/abort logs include route, bytes sent and expected bytes;
slow successful transmission is reported separately from slow request rendering.
Explicit restart/factory-reset actions execute after the response finishes or
aborts, followed by a cooperative 200-ms grace period. Synchronous presenters
retain immediate completion semantics.

This applies to normal presented HTML pages (including sensors/controllers/SD)
and CSS. Direct SDK responses, the chunked timezone selector, uploads and custom
file-download handlers retain their existing paths; SD downloads already have a
separate nonblocking pump. HTTP request parsing remains the SDK parser. Hardware
validation of WiFi throughput and concurrent SD recording is still required;
this does not establish the cause of a separate SD write failure.

Validation: 39 native shared-web tests pass, including short writes, backpressure,
byte-exact complete pages, bounded per-loop work, timeout/disconnection, unchanged
Content-Length and restart/factory reset after deferred completion. All 71 tester
native cases pass across the complete run and the updated asynchronous fixture
rerun. Both ESP32 targets build; EnvNode measured 1,656,157 program bytes and
101,764 static RAM bytes before the final build-identity refresh. Static RAM
increases by 192 bytes; runtime page/socket memory still needs hardware measurement.
