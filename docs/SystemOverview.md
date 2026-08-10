# WeatherStation System Overview

**Project:** WeatherStation  
**Status:** Active Development  
**Document Version:** August 2026

---

# 1. Vision

WeatherStation is a modular embedded platform for environmental sensing and local automation.

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
weatherstation/<device>/sensor/<sensorId>/<measurement>
```

Properties:

- JSON payload
- non-retained
- best effort

Measurements intentionally remain stateless.

---

# 8. Home Assistant

WeatherStation supports MQTT Device Discovery.

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

# 14. Future Architecture

## Actuators

Future firmware versions will introduce Actuators.

Responsibilities:

- control physical outputs
- receive Commands
- expose state

Initially Actuators are controlled externally.

Examples:

- MQTT
- Web UI

---

## Controllers

Controllers are a planned future domain concept.

Responsibilities:

- consume Measurements
- execute local decision logic
- generate Commands for Actuators

Examples:

- Cistern Controller
- Rain Detector Heater Controller

Controllers enable autonomous local behaviour without Home Assistant.

This concept is intentionally postponed until required by the first real application.

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

The same philosophy will later apply to Actuators and Controllers.

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
- Web Configuration

Currently under development:

- SHT4x
- BMP390

Future:

- ADS1115 Service
- Radiation Sensor
- Pressure Probe
- Actuator Framework
- Controller Framework

---

# 17. Project Philosophy

WeatherStation is designed as a reusable embedded platform.

The objective is not to build a single weather station.

The objective is to build a maintainable platform capable of supporting future measurement and automation applications while preserving a consistent architecture.