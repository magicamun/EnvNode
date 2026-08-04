# Domain Model

## Purpose

This document defines the functional domain model of WeatherStation.

It describes the core concepts of the system independently from their technical implementation.

The purpose of this document is to answer one question:

> Which concepts exist in the WeatherStation domain?

The following topics are intentionally excluded from this document:

- ESP32 hardware
- WiFi
- MQTT
- HTTP
- OTA
- NVS
- PlatformIO
- FreeRTOS
- communication protocols
- implementation details

Those belong to the Technical Architecture.

---

# Core Domain

The WeatherStation domain consists of six fundamental concepts:

- Device
- Configuration
- Sensor
- Measurement
- Actuator
- Diagnostics

Conceptually:

    Device
      │
      ├──────── Configuration
      │
      ├──────── Diagnostics
      │
      ├──────── Sensors
      │              │
      │              └──────── Measurements
      │
      └──────── Actuators

The Device is the root of the domain.

Everything belongs to exactly one Device.

---

# Device

The Device represents one physical WeatherStation installation.

The Device owns:

- Configuration
- Sensors
- Actuators
- Diagnostics

The Device coordinates the system.

The Device does not perform weather interpretation.

Typical properties include:

- device identifier
- device name
- firmware version
- operating state

The Device represents the weather station as a whole.

---

# Configuration

Configuration contains all persistent installation-specific settings.

Typical examples include:

- calibration values
- thresholds
- hardware options
- simulation mode
- installation parameters

Configuration determines how the Device behaves.

Configuration is **not** weather data.

Examples of valid configuration:

- rain gauge calibration
- solar sensor calibration factor
- rain detector threshold
- heater limits

Examples of invalid configuration:

- rainfall today
- sunshine duration
- ETo
- pressure trend

Those values are derived information and belong outside the Device.

---

# Sensor

A Sensor represents a source of physical measurements.

A Sensor may be:

- physical
- simulated

Examples include:

- temperature
- humidity
- pressure
- solar radiation
- rain detector
- rain gauge

Each Sensor has:

- identity
- type
- operational state
- configuration
- one or more Measurements

The Sensor is responsible for obtaining reliable physical values.

A Sensor may perform calculations that are necessary to obtain meaningful physical measurements.

Examples include:

- ADC conversion
- calibration
- temperature compensation
- signal filtering
- debounce

A Sensor must never perform weather interpretation.

---

# Measurement

A Measurement represents one observed physical value or event.

Measurements are independent of concrete sensor implementations.

Examples:

- temperature
- humidity
- pressure
- irradiance
- rain detector value
- rain detector state
- rain gauge tip event

A Measurement conceptually contains:

- identifier
- source
- timestamp
- value
- unit
- validity
- quality
- simulated flag

Not every Measurement requires every property.

A rain gauge tip, for example, is an event.

Temperature is a continuous value.

---

# Measurement Types

Two different categories of Measurements exist.

## State Measurement

Represents a physical quantity.

Examples:

- temperature
- humidity
- pressure
- irradiance
- heater output

Conceptually:

Measurement

- value
- unit
- timestamp

---

## Event Measurement

Represents something that happened.

Examples:

- rain gauge tip
- sensor failure
- sensor recovery

Conceptually:

Event

- type
- source
- timestamp

---

# Measurement Ownership

Measurements belong to Sensors.

A Sensor may produce multiple Measurements.

Example:

Solar Sensor

- irradiance
- cell temperature
- raw ADC value

The Device never owns Measurements directly.

---

# Actuator

An Actuator represents hardware controlled by the Device.

The first planned Actuator is:

- rain detector heater

Future examples might include:

- fan
- relay
- valve
- sensor heater

Each Actuator has:

- identity
- configuration
- state
- output value
- limits

Actuators control hardware.

They do not interpret weather.

---

# Diagnostics

Diagnostics describe the health of the Device and its components.

Diagnostics are not Measurements.

Typical examples include:

- sensor available
- sensor failed
- restart reason
- low memory
- configuration valid
- calibration required

Diagnostics allow failures to be communicated without interrupting unrelated functionality.

---

# Sensor State

Each Sensor has an explicit operational state.

Possible states include:

- Unknown
- Initializing
- Ready
- Degraded
- Failed
- Simulated

Sensor availability must always be explicit.

One failed Sensor must never stop unrelated Sensors.

---

# Measurement Validity

A Measurement is more than a numeric value.

Every Measurement has a validity.

Examples of invalid Measurements:

- sensor not initialized
- communication timeout
- impossible value
- conversion failed

Invalid data must never silently appear as plausible values.

Bad example:

Temperature = 0.0 °C

Good example:

Temperature = unavailable

Reason = sensor failure

---

# Local Hardware Control

Some decisions belong inside the Device because they directly affect hardware.

Examples:

- rain detector heater control
- debounce
- sensor initialization
- hardware protection

These are hardware responsibilities.

They are fundamentally different from weather interpretation.

Example:

Allowed:

Rain detected

↓

Enable heater

Not allowed:

Rain detected

↓

Rain forecast

---

# Simulation

Simulation is a property of Sensors.

A simulated Sensor behaves exactly like a physical Sensor.

It produces the same Measurements.

Consumers must not distinguish between:

- real Sensor
- simulated Sensor

Simulation therefore belongs inside the Sensor abstraction.

---

# Domain Relationships

Device

owns

- Configuration
- Sensors
- Actuators
- Diagnostics

Sensors

produce

Measurements

Actuators

modify

Hardware

Diagnostics

describe

Device state

---

# Domain Boundary

The domain ends with:

- Measurements
- Configuration
- Diagnostics
- Actuator state

The domain intentionally excludes:

- transport
- communication
- persistence
- automation
- visualization

Those belong to the Technical Architecture.

---

# Core Rule

The central rule of the WeatherStation domain is:

Sensors produce Measurements.

Measurements describe physical reality.

Interpretation belongs outside the Device.

Examples:

Domain:

- temperature
- humidity
- pressure
- irradiance
- rain tip
- rain detected

Outside the domain:

- rainfall today
- sunshine duration
- pressure trend
- evapotranspiration
- irrigation requirement

This distinction is fundamental to the WeatherStation architecture.