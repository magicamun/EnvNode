# EnvNode

EnvNode is an open-source ESP32-based embedded platform for reliable environmental measurements and seamless integration into modern home automation systems.

WeatherStation is the first reference application built on the reusable EnvNode platform.

The project intentionally separates **measurement** from **interpretation**.

The firmware is responsible for acquiring reliable physical Measurements and publishing them to external systems.

Higher-level concepts such as weather interpretation, historical aggregation, evapotranspiration (ETo), irrigation logic and visualization intentionally remain outside the firmware.

EnvNode is developed as a complete open-source product including:

- firmware
- hardware (KiCad)
- documentation
- development tooling

The goal is not to build another weather dashboard.

The goal is to build a reliable measurement platform.

---

# Vision

EnvNode provides an open, transparent and extensible weather sensor platform for Home Assistant and other home automation systems.

The ESP32 acts as a dedicated measurement node.

It acquires physical sensor data, performs only the hardware-related processing required to obtain reliable Measurements and publishes them via MQTT.

The firmware intentionally remains deterministic, maintainable and independent from weather-specific interpretation.

This allows higher software layers to evolve independently while keeping the embedded firmware simple and robust.

---

# Why another weather station?

The project originated from practical experience with an existing DIY weather station.

Although the hardware proved to be reliable, several software aspects became increasingly problematic:

- closed-source firmware
- undocumented algorithms
- incorrect sunshine duration calculation
- embedded rain aggregation
- limited extensibility
- unclear separation between measurement and interpretation

Rather than replacing individual algorithms, the firmware was redesigned from first principles around a small number of architectural rules.

---

# Core Philosophy

The EnvNode project follows one simple principle:

> Measure first.
>
> Interpret later.

The firmware exists to acquire reliable Measurements.

Interpretation belongs to external systems.

---

# Project Goals

- fully open-source firmware
- open hardware (KiCad)
- MQTT-based integration
- browser-based configuration
- OTA firmware updates
- modular architecture
- high-quality Measurements
- deterministic firmware behaviour
- long-term maintainability
- simulation support
- comprehensive documentation
- hardware independence

---

# Design Principles

The project follows a small number of architectural principles.

### Measure, don't interpret

The firmware measures physical reality.

Interpretation belongs outside the Device.

---

### One technical responsibility per service

Each infrastructure service owns exactly one technical responsibility.

Examples include:

- Configuration
- WiFi
- Time
- MQTT

---

### Sensors produce Measurements

Sensors do not publish MQTT.

Sensors do not interpret weather.

Sensors produce Measurements.

### Controllers coordinate Actuators through capabilities

Controllers implement cooperative local behavior and address configured Actuators by `ActuatorId` and capability. They do not manipulate GPIO or use MQTT as an internal control path.

---

### Configuration has one authoritative owner

Configuration exists exactly once inside the runtime.

Every subsystem uses the same Configuration.

---

### Simulation is part of the architecture

Simulated and physical Sensors follow the same processing pipeline.

Replacing a simulated Sensor with a physical implementation must not require architectural changes.

---

### Observable behaviour

Whenever practical, internal decisions should be visible through:

- diagnostics
- logging
- MQTT status

The firmware should never behave like a black box.

---

### Deterministic firmware

The firmware intentionally avoids executing complex weather models.

Embedded software should remain predictable and responsive.

---

### Hardware independence

Application logic depends on abstract Sensor behaviour rather than concrete hardware implementations.

Replacing a Sensor should require only a new driver.

---

# Current Status

Implemented

- ESP32 PlatformIO project
- persistent configuration
- browser-based provisioning
- WiFi connectivity
- automatic WiFi reconnect
- setup access point
- configurable hostname
- SNTP time synchronization
- configurable timezone
- MQTT connectivity
- authenticated MQTT client
- factory reset and OTA firmware update
- typed Measurement pipeline and MQTT publication
- fixed Sensor slots, physical/simulated implementations and live runtime composition
- Sensor Home Assistant discovery
- fixed Actuator slots and unified hardware validation
- OnOff capability and GPIO On/Off Actuator
- live Actuator runtime rebuild
- Web and MQTT Actuator configuration/control
- fixed Controller slots and BlinkController
- ThresholdController with typed Measurement input, hysteresis and monotonic freshness
- cooperative Controller runtime and live rebuild
- Web and MQTT Controller configuration, status and Start/Stop
- persistent MQTT Blink/Threshold parameter commands and retained parameter state
- exclusive enabled-Controller ownership of Actuator targets
- physically verified Sensor -> Measurement -> Controller -> Actuator operation

Deliberately future

- additional Actuator capabilities and typed Controller implementations
- multi-input, Boolean/contact and event-driven Controllers
- richer manual-versus-Controller arbitration
- generic external Actuator/Controller self-description
- Controller and generic Actuator Home Assistant discovery

---

# Documentation

Project documentation is intentionally separated by responsibility.

| Document | Purpose |
|----------|---------|
| README.md | Project overview |
| DomainModel.md | Functional domain concepts |
| TechnicalArchitecture.md | Technical architecture |
| ActuatorModel.md | Actuator capabilities and Controller interaction |
| MQTT.md | Current MQTT protocol boundaries and topics |
| ADRs | Architectural decisions |

---

# Non Goals

EnvNode intentionally does not:

- perform weather forecasting
- calculate evapotranspiration (ETo)
- calculate sunshine duration
- aggregate historical weather data
- store long-term measurements
- replace Home Assistant

These responsibilities belong to higher software layers.

---

# Project Status

The Sensor, Actuator and Controller architecture is implemented and physically verified. Current open work concerns further capabilities, Controller implementations and external self-description rather than replacing these runtime boundaries.
