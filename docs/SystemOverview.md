# EnvNode System Overview

**Project:** EnvNode
**Status:** Active Development  
**Document Version:** August 2026

---

# 1. Vision

EnvNode is a modular embedded platform for environmental sensing and local automation.

Although the initial focus is weather observation, the architecture is intentionally designed to support a much broader class of measurement and automation applications.

Examples include:

- Weather Stations
- Rain Gauges
- Solar Radiation Measurement
- Water Tank Monitoring
- Environmental Monitoring
- Future Local Automation Nodes

The long-term objective is to maintain **one common firmware platform** rather than multiple independent device-specific firmware projects.

---

# 2. Design Philosophy

The project follows one fundamental principle:

> **Measure first – interpret later.**

The firmware should primarily perform accurate physical measurements.

Interpretation, aggregation and visualization belong outside the firmware whenever possible.

Examples:

Firmware:

- Temperature
- Relative Humidity
- Atmospheric Pressure
- Rainfall Increment

External Systems (Home Assistant):

- Daily Rainfall
- Monthly Rainfall
- Irrigation Decisions
- Historical Statistics
- Dashboards

Only functionality that must operate autonomously or safely without external systems is implemented locally.

---

# 3. System Architecture

The architecture is based on clear domain responsibilities.

```
                +------------------+
                |     Sensor       |
                +------------------+
                          |
                          v
                   Measurements
                          |
                          v
                +------------------+
                | Sensor Manager   |
                +------------------+
                   |            |
                   |            |
                   v            v
          MQTT Publisher   Runtime Diagnostics
                   |
                   v
                 MQTT
                   |
          +--------+--------+
          |                 |
          v                 v
 Home Assistant       Other Consumers
```

Every component has exactly one responsibility.

---

# 4. Domain Model

## Sensor

A Sensor represents one physical measurement device.

Responsibilities:

- initialize hardware
- read hardware
- generate Measurements

Sensors do **not** perform long-term aggregation or automation logic.

Examples:

- AM2302
- SHT4x
- BMP390
- Rain Gauge
- Pressure Probe

---

## Measurement

Measurements are the central data object of the system.

A Measurement contains information such as:

- MeasurementType
- Value
- Unit
- Timestamp
- Quality
- Validity
- Source Sensor

Measurements represent observations only.

They do not represent business logic.

---

## Measurement Metadata

Every MeasurementType owns authoritative metadata.

Examples:

- display name
- canonical unit
- value kind
- recommended precision
- state/event semantics

These metadata are consumed by:

- Web UI
- MQTT
- Home Assistant Discovery
- future protocol adapters

---

# 5. Runtime Model

Sensor configuration is persistent.

Runtime composition is dynamic.

Whenever configuration changes are applied:

```
Persistent Configuration
            |
            v
   Sensor Factory
            |
            v
 Runtime Sensor Instances
```

The runtime can be rebuilt without rebooting the device.

This enables:

- GPIO reassignment
- Sensor replacement
- Sensor enable/disable

without restarting the ESP32.

---

# 6. Sensor Identity

Every configured Sensor owns:

- Slot
- SensorId
- User-defined Name

MQTT topics use the stable SensorId.

User-visible names may change without affecting integrations.

---

# 7. MQTT

Measurements are published using:

```
envnode/<device>/sensor/<sensorId>/<measurement>
```

Properties:

- JSON payload
- non-retained
- best effort

Measurements intentionally remain stateless.

---

# 8. Home Assistant

EnvNode supports MQTT Device Discovery.

Discovery is retained.

Measurements remain non-retained.

The firmware automatically republishes Discovery when required.

No YAML configuration is required.

---

# 9. Hardware Abstraction

Hardware is represented as reusable resources.

Current resources include:

- GPIO
- I²C

Future resources:

- ADC
- OneWire
- SPI

Resources are managed independently from individual Sensor implementations.

---

# 10. Current Sensors

Implemented:

- Simulated Sensors
- AM2302
- Rain Gauge

Planned:

- SHT4x
- BMP390
- Radiation Sensor
- Pressure Probe

---

# 11. ADC Architecture

The ADS1115 is considered a hardware resource rather than a Sensor.

```
Sensor
      |
      v
ADS1115 Service
      |
      v
Analog Channel
```

Multiple Sensors may share the same ADS1115.

Examples:

- Radiation Sensor
- Rain Detector
- Pressure Probe

---

# 12. Build Identity

Firmware versioning consists of two independent concepts.

## Product Version

Semantic version controlled intentionally by developers.

Example:

```
0.1.0
```

## Build Identity

Automatically generated from Git.

Example:

```
0.1.0+317.g7f8c2d1
```

Including:

- Build Number
- Commit SHA
- Branch
- Dirty State
- Build Timestamp

This guarantees complete firmware traceability.

---

# 13. Diagnostics

The firmware exposes runtime diagnostics including:

- Runtime Sensors
- Last Measurement
- Measurement Browser
- Firmware Information
- Build Identity
- Home Assistant Discovery

The goal is to diagnose devices directly from the embedded web interface.

---

# 14. Actuator and Controller Architecture

## Actuators

Configured Actuator Slots expose typed physical-output capabilities. `ActuatorRuntime` owns runtime instances and resolves capability by `ActuatorId`; `gpio_on_off` exposes `OnOff` through `IOnOffActuator`, while `gpio_pwm` exposes normalized 0–100 `Level` through `ILevelActuator` and also satisfies `OnOff`. Web, MQTT and Controllers never manipulate GPIO directly.

---

## Controllers

Controllers are configured local behaviors owned by `ControllerRuntime`. Blink provides cooperative non-blocking timing. Threshold consumes one `MeasurementSourceReference` through copied snapshots, applies hysteresis and commands an `OnOff` target. One enabled Controller may own each target; runtime STOP does not release configured ownership.

Web and MQTT are adapters over the same configuration and runtime model. Manual Actuator commands remain last-command-wins relative to a Controller, while Controller-versus-Controller target conflicts are rejected.

Retained schema-1 descriptions provide external self-description for configured Actuators and Controllers. Future typed implementations may add multi-input RainDetector behavior, Boolean/contact or event semantics. Controller and generic Actuator Home Assistant discovery and richer manual arbitration are not implemented.

---

# 15. Development Principles

The architecture is intentionally extensible.

Adding a new Sensor should normally require only:

- Sensor implementation
- Registry entry

without modifying:

- Sensor Manager
- MQTT
- Discovery
- Runtime
- Web infrastructure

The same registry, typed-interface and composition principles apply to Sensors, Actuators and Controllers while their domain lifecycles remain separate.

---

# 16. Current Project Status

Implemented:

- Sensor Registry
- Sensor Factory
- Runtime Rebuild
- Measurement Pipeline
- MQTT Publisher
- Home Assistant Discovery
- Runtime Diagnostics
- Build Identity
- EEPROM-backed Board Identity, boot-time resolution and Web provisioning
- Web Configuration
- Actuator Slots, OnOff and Level capabilities, GPIO On/Off and PWM implementations, and live ActuatorRuntime
- Controller Slots, BlinkController and ThresholdController
- MeasurementSourceReference and monotonic freshness
- live ControllerRuntime rebuild and transient START/STOP
- Web and MQTT Controller configuration/status/parameters
- retained external Actuator and Controller descriptions
- exclusive enabled-Controller target ownership

Current hardware-validation focus:

- Board Identity EEPROM provisioning and boot selection on a physical Mainboard revision 0.3
- physical bring-up and production validation of the current Mainboard and Module revision 0.3 designs
- integration and calibration of a concrete ADC backend and pressure probe on real hardware

Future firmware extensions:

- ADS1115 Service
- Radiation Sensor
- Pressure Probe
- additional typed Controller implementations
- Controller and generic Actuator Home Assistant discovery

---

# 17. Project Philosophy

EnvNode is designed as a reusable embedded platform.

The objective is not to build a single weather station.

The objective is to build a maintainable platform capable of supporting future measurement and automation applications while preserving a consistent architecture.
