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

The Device is the aggregate root of the WeatherStation domain.

Everything belongs to exactly one Device.

Measurements are the primary domain objects produced by the Device.

The WeatherStation exists to acquire reliable Measurements from the physical world.

All other domain concepts ultimately support that goal.

---

# Device

A Device represents one physical WeatherStation installation.

The Device owns:

- Configuration
- Sensors
- Actuators
- Diagnostics

The Device coordinates the complete WeatherStation.

Typical properties include:

- device identifier
- device name
- firmware version
- operating state

The Device intentionally does not own Measurements directly.

Measurements are owned by the Sensors that produce them.

The Device does not interpret weather data.

Its responsibility is limited to acquiring, coordinating and exposing reliable Measurements.

---

# Configuration

Configuration contains all installation-specific parameters that influence Device behaviour.

Configuration represents long-lived operational settings rather than runtime state.

Typical examples include:

- calibration values
- thresholds
- hardware options
- simulation mode
- installation parameters
- network configuration
- time configuration

Configuration determines how the Device behaves.

Configuration is never considered Measurement data.

Examples of valid Configuration include:

- rain gauge calibration
- solar sensor calibration factor
- rain detector threshold
- heater limits
- MQTT server
- WiFi credentials
- timezone

Examples of invalid Configuration include:

- rainfall today
- sunshine duration
- evapotranspiration
- pressure trend

Those values are derived information and therefore remain outside the Device domain.

---

# Sensor

A Sensor represents one physical or simulated measurement source.

A Sensor is not identical to a Measurement type.

One Sensor may produce one or more different Measurements.

Examples:

BMP390

- pressure
- temperature

SHT4x

- temperature
- humidity

Solar Sensor

- irradiance
- cell temperature

Rain Detector

- normalized detector level
- wet/dry state

Rain Gauge

- tip event

A Sensor therefore represents the measurement-producing component, not the individual physical quantity.

Each Sensor has:

- identity
- type
- operational state
- provenance
- configuration
- one or more supported Measurement types

Sensor identity is represented by a `SensorId`.

A `SensorId` is an unsigned 16-bit value.

The value `0` is reserved for invalid or unassigned identity.

Every nonzero SensorId is unique within one Device and identifies one logical measurement source.

All Measurement types produced by the same Sensor use the same SensorId.

The Sensor is responsible for obtaining reliable physical values from its measurement source.

A Sensor may perform calculations that are technically required to derive a meaningful physical Measurement.

Examples include:

- ADC conversion
- calibration
- temperature compensation
- signal filtering
- debounce
- conversion from raw hardware values into engineering units

These operations belong to measurement acquisition.

A Sensor must never perform weather interpretation.

Examples of allowed Sensor behaviour:

    raw ADC value
        |
        v
    calibrated irradiance Measurement

and:

    rain gauge switch transition
        |
        v
    debounced rain gauge tip event

Examples of behaviour that does not belong to a Sensor:

- rainfall today
- sunshine duration
- pressure trend
- evapotranspiration
- weather classification

A physical Sensor and a simulated Sensor must expose the same functional behaviour to the rest of the domain.

Sensor provenance is independent from operational state.

Possible provenance values are:

- Physical
- Simulated

A simulated Sensor may therefore be Ready, Degraded or Failed in the same way as a physical Sensor.

---

# Measurement

A Measurement represents one observed physical value or one physical event.

Measurements are the primary output of Sensors.

A Measurement is independent from the concrete hardware implementation that produced it.

Examples include:

- temperature
- humidity
- pressure
- irradiance
- solar cell temperature
- normalized rain detector level
- rain detector wet/dry state
- rain gauge tip event

A Measurement conceptually contains:

- MeasurementType
- source SensorId
- Unix Epoch timestamp
- MeasurementValue or explicit no-value event
- validity
- MeasurementQuality
- SensorProvenance

Every accepted Measurement contains this metadata, but its value kind depends on the Measurement Type.

A temperature Measurement contains a numeric value in the canonical unit defined by its Measurement type.

A rain gauge tip Measurement represents an event and therefore has no value.

Event Measurements must never use artificial numeric values.

Measurement values use a small tagged representation with the following possible kinds:

- None
- FloatingPoint
- Boolean
- UnsignedInteger

The None kind represents a value-free event or an invalid state Measurement without a usable value.

The value representation must remain compatible with the firmware C++ standard and use deterministic memory.

The purpose of Measurement is to describe physical reality in a normalized domain representation.

Consumers of Measurements must not need to understand:

- GPIO pins
- I2C addresses
- ADC channels
- register layouts
- hardware-specific scaling
- whether the source is simulated or physical

Measurements may represent either a continuous physical quantity or a discrete physical event.

Examples of continuous Measurements include:

- temperature
- humidity
- pressure
- irradiance

Examples of event Measurements include:

- rain gauge tip

Both are Measurements.

Consumers should not require different processing pipelines simply because one Measurement represents a value while another represents an event.

---

# Measurement Type

Measurement Type identifies what a Measurement represents.

Examples include:

- unknown
- temperature
- relative humidity
- atmospheric pressure
- solar irradiance
- solar cell temperature
- normalized rain detector level
- rain detector wet/dry state
- rain gauge tip

The initial Measurement Types and their canonical representations are:

| Measurement Type | Value Kind | Canonical Representation |
|------------------|------------|--------------------------|
| Unknown | None | invalid or unassigned type |
| Temperature | FloatingPoint | degrees Celsius |
| RelativeHumidity | FloatingPoint | percent, 0 to 100 |
| AtmosphericPressure | FloatingPoint | Pascal |
| SolarIrradiance | FloatingPoint | watt per square metre |
| SolarCellTemperature | FloatingPoint | degrees Celsius |
| RainDetectorLevel | FloatingPoint | normalized ratio, 0.0 to 1.0 |
| RainDetectorWet | Boolean | wet/dry state, no unit |
| RainGaugeTip | None | event, no unit |

Units are metadata of Measurement Type.

Measurements do not carry arbitrary unit values.

Alternative presentation units are the responsibility of external consumers.

Raw ADC values are generally Diagnostics rather than primary Measurement Types.

Measurement Type is independent from Sensor identity.

This distinction is important because different Sensors may produce the same Measurement Type.

Example:

    SHT4x
        |
        +----> temperature

    BMP390
        |
        +----> temperature

Both Measurements represent temperature.

They remain distinguishable through their source Sensor.

Measurement Type therefore answers:

> What was measured?

Sensor identity answers:

> Where did the Measurement come from?

---

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
- canonical unit defined by Measurement Type
- timestamp

---

## Event Measurement

Represents something that happened.

Examples:

- rain gauge tip

Conceptually:

Event

- type
- source
- timestamp

---

# Measurement Ownership

Measurements belong to the Sensor that produced them.

A Sensor may produce one or more different Measurement Types.

Examples:

BMP390

- pressure
- temperature

SHT4x

- temperature
- humidity

Solar Sensor

- irradiance
- cell temperature

Rain Detector

- normalized detector level
- wet/dry state

The Device never owns Measurements directly.

Instead, the Device owns Sensors, and Sensors produce Measurements.

Measurements are intentionally transient domain objects.

Sensors supply the physical content, validity and quality of Measurements.

When SensorManager accepts Measurement content, it always copies the Sensor identity from `sensor.id()`, always assigns provenance from `sensor.provenance()`, validates structural Measurement Type and value-kind compatibility, and assigns a synchronized Unix Epoch timestamp.

SensorManager does not own hardware-specific range validation, conversion or physical plausibility rules.

Those remain Sensor responsibilities.

The primary purpose of the Device is to acquire Measurements rather than permanently storing them.

Long-term storage, aggregation and historical analysis belong outside the WeatherStation domain.

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

Diagnostics describe the operational health of the Device and its components.

Diagnostics are domain objects independent from Measurements.

Typical examples include:

- sensor state
- sensor failed
- sensor recovered
- hardware communication failed
- restart reason
- low memory
- configuration valid
- calibration required

Diagnostics communicate operational state.

Measurements communicate physical reality.

These responsibilities are intentionally separated.

Example:

Temperature sensor failure

↓

Diagnostic

NOT

Temperature = 0°C

Likewise:

A valid temperature Measurement does not imply that every subsystem is healthy.

Diagnostics allow failures to be communicated without interrupting unrelated functionality.

Sensor failure, sensor recovery and hardware communication failure are Diagnostics, not Measurement Types.

---

# Sensor State

Each Sensor has an explicit operational state.

Possible states include:

- Unknown
- Initializing
- Ready
- Degraded
- Failed

Availability is derived from Sensor State rather than stored independently.

- Ready and Degraded Sensors are available.
- Unknown, Initializing and Failed Sensors are unavailable.

One failed Sensor must never stop unrelated Sensors.

---

# Measurement Validity

A Measurement represents more than a numeric value.

Every Measurement has a validity.

Validity answers the question:

> Can this Measurement be used?

Examples of invalid Measurements include:

- sensor not initialized
- communication timeout
- impossible physical value
- conversion failure

Invalid data must never silently appear as plausible values.

An invalid state Measurement has no usable value.

Bad example:

Temperature = 0.0 °C

Good example:

Temperature = unavailable

Reason = sensor failure

The unavailable physical value may be represented by an invalid Measurement when required to clear previously published state.

The failure reason remains a Diagnostic.

Validity is independent from Measurement quality.

A Measurement may be valid while having reduced quality.

Examples include:

- estimated values
- reduced sensor accuracy
- degraded operating conditions

Quality answers the question:

> How trustworthy is this Measurement?

This distinction allows consumers to make informed decisions without treating every imperfect Measurement as unusable.

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

Simulation is represented as Sensor provenance rather than Sensor State.

A simulated Sensor behaves exactly like a physical Sensor.

It produces the same Measurements.

Consumers must not distinguish between:

- physical Sensor
- simulated Sensor

Simulation therefore belongs inside the Sensor abstraction.

The purpose of simulation is to validate the complete WeatherStation domain model before physical hardware is available.

Replacing a simulated Sensor with a physical Sensor must not require architectural changes outside the Sensor implementation.

Simulation validates the complete application behaviour rather than providing a separate execution path.

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

Measurements

describe

physical reality

Diagnostics

describe

operational state

Actuators

modify

hardware state

Configuration

controls

Device behaviour

The relationships intentionally separate:

- physical reality
- operational health
- configuration
- hardware control

Each concept owns one clearly defined domain responsibility.

---

# Domain Boundary

The WeatherStation domain ends with:

- Measurements
- Configuration
- Diagnostics
- Actuator state

The domain intentionally excludes:

- transport
- communication
- persistence implementation
- automation
- visualization
- message serialization
- MQTT topic structures
- HTTP interfaces

These belong to the Technical Architecture.

The domain describes *what* the WeatherStation is.

The Technical Architecture describes *how* the WeatherStation is implemented.

---

# Core Rule

The central rule of the WeatherStation domain is:

Sensors produce Measurements.

Measurements describe physical reality.

Measurements intentionally contain no interpretation.

Interpretation belongs outside the Device.

Examples belonging to the domain:

- temperature
- humidity
- pressure
- irradiance
- rain tip
- rain detected

Examples outside the domain:

- rainfall today
- sunshine duration
- pressure trend
- evapotranspiration
- irrigation requirement

The Device measures.

External systems understand.

This distinction is fundamental to the WeatherStation architecture.
