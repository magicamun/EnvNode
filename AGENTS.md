# AGENTS.md

# WeatherStation Development Guidelines

This document defines the architectural rules and development principles for the WeatherStation project.

These rules are intentionally stricter than normal coding guidelines.

Whenever implementation convenience conflicts with these rules, the architecture takes precedence.

---

# Project Mission

WeatherStation is an open-source ESP32-based weather sensor platform.

Its purpose is to provide reliable physical measurements and expose them through a clean and transparent interface.

The firmware is intentionally **not** a weather analysis engine.

---

# Core Principle

> Measure first.
>
> Interpret later.

Whenever deciding whether functionality belongs inside the firmware, prefer publishing reliable measurements instead of calculated values.

---

# Responsibilities

The firmware is responsible for:

- sensor acquisition
- sensor calibration
- hardware control
- local safety functions
- MQTT communication
- device configuration
- diagnostics
- OTA updates
- web configuration

The firmware is NOT responsible for:

- weather forecasting
- irrigation logic
- evapotranspiration (ETo)
- sunshine duration
- historical aggregation
- rainfall statistics
- long-term storage
- automation logic

These functions belong to higher software layers.

---

# Separation of Concerns

Every module shall have exactly one responsibility.

Examples:

- ConfigManager manages configuration.
- WifiManager manages WiFi.
- MqttManager manages MQTT.
- SensorManager manages sensors.
- WebServer manages HTTP.
- OTA manages firmware updates.

Avoid mixing responsibilities.

---

# Sensor Architecture

Application logic must never depend directly on hardware drivers.

Always depend on abstract interfaces.

Good:

Application

↓

SensorManager

↓

ISensor

↓

SHT45

Bad:

Application

↓

SHT45

This allows:

- simulation
- testing
- hardware replacement
- cleaner architecture

---

# Configuration

Configuration shall be stored in ESP32 non-volatile storage (NVS).

Configuration shall survive:

- reboot
- power loss
- firmware update

Do not hard-code installation-specific parameters.

Everything that may differ between installations should be configurable.

---

# MQTT

MQTT is the primary integration interface.

MQTT shall publish:

- measurements
- device status
- diagnostics
- availability

MQTT must not contain business logic.

The firmware publishes measurements.

Consumers decide what they mean.

---

# Local Processing

The firmware may perform calculations only when they are required to obtain meaningful physical measurements.

Examples:

Allowed

- ADC conversion
- temperature compensation
- sensor calibration
- irradiance calculation
- debouncing
- averaging required by the sensor

Not allowed

- daily rain
- sunshine duration
- pressure trend
- ETo
- weather classification

---

# Failure Behaviour

One failing sensor must never stop the complete device.

Failures shall be isolated.

Whenever possible:

- continue measuring
- continue publishing
- report the failure

Graceful degradation is preferred over complete shutdown.

---

# Simulation

The firmware shall support simulated sensors.

Simulation must use the same interfaces as real hardware.

MQTT and application code must not know whether a value originates from:

- real hardware
- simulated hardware

---

# Diagnostics

The firmware must never behave as a black box.

Expose useful diagnostics whenever practical.

Examples:

- firmware version
- uptime
- restart reason
- WiFi RSSI
- free memory
- sensor status
- sensor errors
- raw ADC values where useful

---

# Logging

Logging should be meaningful.

Good logging explains:

- what happened
- why it happened
- what the firmware decided

Avoid excessive debug output.

Prefer structured and readable messages.

---

# Hardware

Hardware is developed in KiCad.

Hardware and firmware evolve together but remain logically separated.

The repository contains:

- firmware
- hardware
- documentation

None of these is considered secondary.

---

# Repository Structure

WeatherStation/

├── firmware/

├── hardware/

├── docs/

├── images/

README.md

AGENTS.md

Every new feature should update documentation where appropriate.

---

# Coding Style

Prefer:

- readable code
- explicit names
- small classes
- small functions
- composition over inheritance
- interfaces over concrete implementations

Avoid:

- unnecessary global state
- duplicated code
- hidden side effects
- tightly coupled modules

---

# Dependencies

Introduce external libraries only when they provide clear value.

Keep dependencies minimal.

Prefer mature and actively maintained libraries.

---

# Testing

Develop incrementally.

Every new subsystem should be testable independently.

Typical order:

1. Configuration

2. WiFi

3. MQTT

4. Web Interface

5. OTA

6. Diagnostics

7. Sensor Drivers

8. Complete Integration

Do not integrate multiple unfinished subsystems simultaneously.

---

# Architecture First

When uncertain where functionality belongs:

1. Update the architecture.

2. Then update the implementation.

Never solve architectural problems by adding special-case code.

---

# Documentation

Documentation is part of the project.

Architecture documentation is considered as important as source code.

Keep documentation synchronized with implementation.

---

# Final Rule

Whenever a design decision is unclear, ask:

"Does this improve measurement quality, or is it interpretation?"

If it is interpretation, it probably belongs outside the firmware.