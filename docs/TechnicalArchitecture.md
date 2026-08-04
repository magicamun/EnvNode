# Technical Architecture

## Purpose

This document defines the technical architecture of WeatherStation.

It describes how the domain model is implemented on the embedded platform and how the different technical layers interact.

The domain model itself is defined separately in `DomainModel.md`.

This document intentionally focuses on technical responsibilities such as:

- runtime
- hardware access
- connectivity
- communication
- persistence
- web access
- firmware updates
- diagnostics

The goal is to keep the technical architecture modular and independent from the concrete sensor hardware wherever possible.

---

# Architectural Layers

WeatherStation is divided into four technical layers:

1. Application Layer
2. Communication Layer
3. Connectivity and Infrastructure Layer
4. Hardware Layer

Conceptually:

    +--------------------------------------------------+
    |                Application Layer                 |
    |--------------------------------------------------|
    | Device orchestration                             |
    | Sensor coordination                              |
    | Configuration use                                |
    | Diagnostics                                      |
    | Local hardware control                           |
    +-------------------------+------------------------+
                              |
                              v
    +--------------------------------------------------+
    |              Communication Layer                 |
    |--------------------------------------------------|
    | MQTT                                             |
    | HTTP / Web Interface                             |
    | OTA                                              |
    +-------------------------+------------------------+
                              |
                              v
    +--------------------------------------------------+
    |       Connectivity and Infrastructure Layer      |
    |--------------------------------------------------|
    | WiFi                                             |
    | Time / NTP                                       |
    | Persistent storage                               |
    | Logging                                          |
    +-------------------------+------------------------+
                              |
                              v
    +--------------------------------------------------+
    |                  Hardware Layer                  |
    |--------------------------------------------------|
    | GPIO                                             |
    | I2C                                              |
    | ADC                                              |
    | OneWire                                          |
    | PWM                                              |
    | ESP32 hardware                                   |
    +--------------------------------------------------+

Dependencies should point downward.

Higher layers may depend on lower layers.

Lower layers must not depend on application-specific logic.

---

# Application Layer

The Application Layer coordinates the WeatherStation as a whole.

It connects the domain model to technical services.

Typical responsibilities include:

- initializing the device
- loading configuration
- initializing sensors
- coordinating measurements
- coordinating actuators
- exposing diagnostics
- forwarding measurements to communication components
- managing lifecycle state

The Application Layer must not contain hardware-driver logic.

It must also not contain transport-specific implementation details.

For example, application logic should not directly call ESP32 WiFi APIs or manipulate MQTT packets.

Those responsibilities belong to dedicated lower-level components.

---

# Communication Layer

The Communication Layer exposes WeatherStation data and functionality to external systems.

Communication protocols are independent from the transport used underneath them.

The initial communication mechanisms are:

- MQTT
- HTTP
- OTA

---

## MQTT

MQTT is the primary integration protocol.

MQTT operates on top of network connectivity provided by the Connectivity Layer.

Conceptually:

    Measurement
        |
        v
    MQTT representation
        |
        v
    MQTT client
        |
        v
    WiFi
        |
        v
    Network

MQTT is responsible for:

- publishing measurements
- publishing device state
- publishing diagnostics
- publishing availability
- receiving configuration-related commands where explicitly supported

MQTT is not responsible for:

- acquiring measurements
- interpreting weather data
- maintaining WiFi connectivity
- storing long-term data

MQTT must continue attempting reconnection if the broker becomes unavailable.

Loss of MQTT must not stop sensor acquisition or local hardware control.

---

## HTTP / Web Interface

HTTP provides local device administration and diagnostics.

The web interface is primarily intended for:

- initial configuration
- WiFi configuration
- MQTT configuration
- device status
- diagnostics
- calibration parameters
- restart
- factory reset
- firmware update

The web interface is not intended to become a weather dashboard.

Historical visualization and weather interpretation belong to external systems.

HTTP operates on top of the existing network connectivity.

The Web Interface must not own configuration data itself.

It reads and modifies configuration through the configuration subsystem.

---

## OTA

Firmware updates are part of the Communication Layer because they require an external communication path.

Two update mechanisms are planned:

- OTA update from the development environment
- firmware upload through the local web interface

OTA must not own application configuration.

Firmware updates should preserve persistent configuration unless an explicit migration requires otherwise.

USB flashing remains the recovery mechanism if OTA fails.

---

# Connectivity and Infrastructure Layer

The Connectivity and Infrastructure Layer provides reusable technical services required by the Communication and Application Layers.

Initial components include:

- WiFi
- persistent storage
- time synchronization
- logging
- system diagnostics

---

# WiFi

WiFi provides network transport.

WiFi is not an application protocol.

It provides connectivity for:

- MQTT
- HTTP
- OTA
- NTP

Conceptually:

    MQTT -----+
              |
    HTTP -----+----> WiFi ----> Network
              |
    OTA ------+
              |
    NTP ------+

WiFi is responsible for:

- connecting to the configured network
- monitoring connection state
- reconnecting after connection loss
- exposing network diagnostics
- providing fallback setup access when required

WiFi must not contain MQTT-specific or HTTP-specific logic.

---

# WiFi Provisioning

The setup access point is a provisioning and startup-recovery mechanism.

It is not a general fallback mode for temporary network outages.

If no valid WiFi configuration exists, the device shall start a local setup access point.

If valid WiFi configuration exists, the device attempts to connect to the configured network during startup.

If the configured network cannot be reached within a defined startup timeout, the device may start the setup access point to allow recovery from an invalid or unavailable configuration.

Conceptually:

    Boot
      |
      v
    Load WiFi configuration
      |
      +---- missing / invalid ------> Setup Access Point
      |
      +---- valid
              |
              v
         Connect to WiFi
              |
              +---- success -------> Normal Operation
              |
              +---- startup timeout -> Setup Access Point

Once the device has successfully entered normal operation, temporary WiFi outages must not activate the setup access point.

During normal operation:

    WiFi connection lost
            |
            v
    Continue local operation
            |
            v
    Retry WiFi connection indefinitely

The device remains in its normal configured mode while reconnecting.

Likewise, MQTT availability has no influence on WiFi provisioning.

An unavailable MQTT broker must only cause MQTT reconnect attempts.

It must never activate the setup access point.

This distinction prevents temporary infrastructure outages from unexpectedly changing the network behaviour of the device.

# Persistent Storage

Persistent storage is used for configuration.

The initial implementation uses ESP32 non-volatile storage.

Persistent data includes:

- device identity
- WiFi configuration
- MQTT configuration
- calibration values
- sensor settings
- actuator settings
- simulation settings

Persistent storage is not intended for:

- historical weather measurements
- long-term logging
- daily rainfall
- sunshine duration
- time-series data

These belong outside the embedded device.

---

# Configuration Ownership

Configuration has exactly one authoritative in-memory representation.

All technical components obtain configuration from the configuration subsystem.

Conceptually:

    Persistent Storage
           |
           v
      Configuration
           |
       +---+---+--------+--------+
       |       |        |        |
       v       v        v        v
     WiFi     MQTT    Sensors   Web

The Web Interface modifies Configuration.

The Configuration subsystem persists Configuration.

Individual services must not maintain independent copies of persistent settings unless required internally for runtime operation.

This avoids multiple competing sources of truth.

---

# Configuration Lifecycle

The expected configuration lifecycle is:

    Boot
      |
      v
    Load persistent configuration
      |
      v
    Validate configuration
      |
      v
    Create runtime configuration
      |
      v
    Start services
      |
      v
    Configuration changed through Web/API
      |
      v
    Validate
      |
      v
    Persist
      |
      v
    Apply dynamically or restart if required

A configuration change must never be persisted without validation.

---

# Time

Time synchronization is provided by NTP.

Time is infrastructure.

It may be used for:

- timestamps
- diagnostics
- log entries
- event timestamps

The device should continue operating if NTP is temporarily unavailable.

Sensor acquisition must not depend on successful NTP synchronization unless absolute time is technically required.

Internal timing for hardware behaviour should use monotonic timers rather than wall-clock time.

This avoids problems caused by:

- NTP corrections
- daylight-saving changes
- clock jumps

---

# Logging

Logging is an infrastructure service.

Logging should provide useful information about:

- startup
- configuration
- network state
- MQTT state
- sensor initialization
- sensor failures
- actuator state
- OTA
- unexpected conditions

Logging should support different severity levels.

Typical levels are:

- ERROR
- WARN
- INFO
- DEBUG

Production behaviour should not depend on logging output.

Logging must not become a substitute for explicit diagnostics.

---

# Diagnostics

Diagnostics span multiple technical layers.

Each subsystem is responsible for reporting its own operational state.

Examples:

WiFi:

- connected
- disconnected
- RSSI
- IP address

MQTT:

- connected
- disconnected
- reconnecting

Sensors:

- ready
- degraded
- failed
- simulated

System:

- uptime
- free memory
- restart reason
- firmware version

Diagnostics are collected by the application and exposed through available communication interfaces.

---

# Hardware Layer

The Hardware Layer provides direct access to the physical ESP32 and connected components.

Examples include:

- GPIO
- I2C
- OneWire
- ADC
- PWM
- interrupts
- timers

Concrete sensor drivers belong close to this layer.

Application code must not directly access hardware primitives.

For example:

Bad:

    Application
        |
        v
    digitalRead(GPIO27)

Preferred:

    Application
        |
        v
    RainGauge
        |
        v
    GPIO / Interrupt

This keeps hardware-specific behaviour isolated.

---

# Hardware Abstraction

Concrete hardware implementations should expose interfaces that represent their function rather than their electrical implementation.

Examples:

    TemperatureSensor

rather than:

    I2CDeviceAtAddress0x44

and:

    RainGauge

rather than:

    GPIO27InterruptHandler

Electrical implementation details remain inside drivers.

This supports hardware replacement without changing application logic.

---

# Sensor Drivers

A sensor driver bridges physical hardware and the domain model.

Its responsibility is to convert hardware interaction into Measurements.

Conceptually:

    Physical Sensor
          |
          v
    Hardware Driver
          |
          v
       Sensor
          |
          v
     Measurement

Examples of driver responsibilities include:

- I2C communication
- reading registers
- ADC conversion
- hardware-specific filtering
- calibration
- hardware error detection

Drivers must not contain MQTT or Web Interface logic.

---

# Runtime Model

WeatherStation uses a long-running embedded runtime.

The top-level execution model remains intentionally simple.

Conceptually:

    setup()
       |
       v
    Application.initialize()

    loop()
       |
       v
    Application.run()

The exact implementation may later use:

- cooperative scheduling
- timers
- FreeRTOS tasks
- asynchronous callbacks

The application architecture must not depend on one specific scheduling mechanism.

---

# Startup Sequence

The intended startup sequence is:

    Power On
       |
       v
    Initialize basic logging
       |
       v
    Load persistent configuration
       |
       v
    Validate configuration
       |
       v
    Initialize infrastructure
       |
       +--> WiFi
       +--> Time
       +--> Diagnostics
       |
       v
    Initialize communication
       |
       +--> HTTP
       +--> MQTT
       +--> OTA
       |
       v
    Initialize sensors
       |
       v
    Initialize actuators
       |
       v
    Enter normal operation

Not every subsystem must block startup until fully available.

For example, MQTT connection may be established asynchronously while sensor acquisition already starts.

---

# Normal Operation

Normal operation consists of independent recurring activities.

Conceptually:

    Sensor Acquisition
           |
           v
      Measurements
           |
           +-----------------> MQTT
           |
           +-----------------> Web diagnostics

    WiFi maintenance
           |
           v
      reconnect if required

    MQTT maintenance
           |
           v
      reconnect if required

    Hardware control
           |
           v
      actuator updates

    Diagnostics
           |
           v
      monitor component state

These activities should remain loosely coupled.

Failure of one activity must not block unrelated activities.

---

# Event-Driven and Periodic Behaviour

WeatherStation supports both periodic and event-driven data.

Periodic examples:

- temperature
- humidity
- pressure
- solar irradiance

Event-driven examples:

- rain gauge tip
- sensor failure
- sensor recovery

The architecture must support both without forcing event data into artificial polling intervals.

---

# Local Control

Local hardware control must remain functional without network connectivity.

Example:

    Rain Detector
         |
         v
    Heater Controller
         |
         v
    Heater

This path must not require:

- MQTT
- Home Assistant
- Internet access

Local hardware functionality has priority over remote integration.

---

# Failure Isolation

Technical components must fail independently where possible.

Examples:

MQTT broker unavailable:

    WiFi remains connected
    Sensors continue measuring
    Web Interface remains available
    MQTT reconnects in background

One sensor unavailable:

    Remaining sensors continue measuring
    Diagnostics report failure

NTP unavailable:

    Measurement continues
    Time synchronization retries later

Web Interface failure:

    MQTT and measurement continue

WiFi temporarily unavailable:

    Local hardware functions continue
    Sensors continue operating
    WiFi reconnects in the background
    Setup Access Point is not activated

MQTT broker unavailable:

    WiFi remains in normal configured mode
    Sensors continue measuring
    Web Interface remains available if WiFi is connected
    MQTT reconnects in the background
    Setup Access Point is not activated


The application must avoid a single global failure state for recoverable subsystem failures.
---

# Watchdog and Blocking Behaviour

Long blocking operations should be avoided.

Network reconnect attempts must not block:

- sensor acquisition
- hardware control
- diagnostics

Hardware access should use bounded timeouts.

The runtime must remain responsive enough to satisfy the ESP32 watchdog.

The architecture therefore prefers:

- asynchronous behaviour
- short processing steps
- state machines
- timers

over long blocking waits.

---

# Simulation Mode

Simulation uses the same application paths as physical hardware.

Conceptually:

    Real Sensor --------+
                        |
                        v
                    Measurement
                        ^
                        |
    Simulated Sensor ---+

All higher technical layers operate identically regardless of measurement source.

This includes:

- MQTT
- HTTP
- diagnostics
- logging

Simulation must not require a separate communication architecture.

---

# Dependency Direction

Dependencies always point toward lower-level abstractions.

Preferred:

    Application
        |
        v
    Sensor Interface
        |
        v
    Hardware Driver

and:

    Application
        |
        v
    Communication Interface
        |
        v
    MQTT Implementation
        |
        v
    WiFi

Avoid reverse dependencies.

For example, the WiFi component must never call SensorManager.

---

# Dependency Injection

Where practical, dependencies should be supplied explicitly rather than accessed through hidden global state.

Conceptually:

    Application(
        configuration,
        sensors,
        communication,
        diagnostics
    )

This improves:

- testing
- simulation
- modularity
- replaceability

The exact C++ implementation may remain lightweight.

The goal is architectural clarity, not implementation complexity.

---

# Global State

Global mutable state should be minimized.

Some embedded framework objects may require global or static lifetime.

Where this is unavoidable, access should remain encapsulated.

Application data should not be passed implicitly through unrelated global variables.

---

# Memory Management

WeatherStation runs on a memory-constrained embedded system.

The implementation should prefer predictable memory usage.

Guidelines include:

- avoid unnecessary dynamic allocation
- avoid uncontrolled object creation
- reuse buffers where practical
- avoid large temporary JSON documents
- monitor free heap
- treat memory exhaustion as a diagnostic condition

Architecture must remain understandable and safe without introducing unnecessary abstraction overhead.

---

# Security Boundary

WeatherStation is intended for trusted local networks.

Nevertheless:

- WiFi credentials must not be exposed unnecessarily
- MQTT credentials must not appear in normal logs
- passwords must not be returned by diagnostic APIs
- firmware update endpoints should not expose secrets
- configuration handling must validate incoming data

Security may evolve as the project matures, but credentials are always treated as sensitive configuration.

---

# Firmware Versioning

Firmware has an explicit semantic version.

The intended format is:

    MAJOR.MINOR.PATCH

Example:

    0.1.0

The firmware version is exposed through:

- startup logging
- diagnostics
- Web Interface
- MQTT status

Release versions should correspond to Git tags.

Example:

    v0.1.0

There must be one authoritative firmware version definition.

Duplicated manually maintained version strings should be avoided.

---

# Repository Boundary

The WeatherStation repository contains the complete device design.

It includes:

- firmware
- hardware
- documentation
- images and diagrams
- build and development configuration

Conceptually:

    WeatherStation/
    |
    +-- firmware/
    |
    +-- hardware/
    |   |
    |   +-- kicad/
    |   +-- bom/
    |
    +-- docs/
    |
    +-- images/
    |
    +-- README.md
    +-- AGENTS.md

Firmware and hardware are part of one product and therefore belong to the same repository.

---

# Technical Design Rule

The central rule of the technical architecture is:

> Protocols use connectivity.
>
> Application logic uses services.
>
> Services use hardware abstractions.
>
> Hardware does not know the application.

This dependency direction must remain intact as the project evolves.