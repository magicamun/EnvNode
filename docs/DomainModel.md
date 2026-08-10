# Domain Model

## Purpose

This document defines the functional domain model of EnvNode.

It describes the core concepts of the system independently from their technical implementation.

The purpose of this document is to answer one question:

> Which concepts exist in the EnvNode domain?

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

The EnvNode domain consists of six fundamental concepts:

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

The Device is the aggregate root of the EnvNode domain.

Everything belongs to exactly one Device.

Measurements are the primary domain objects produced by the Device.

The EnvNode exists to acquire reliable Measurements from the physical world.

All other domain concepts ultimately support that goal.

---

# Device

A Device represents one physical EnvNode installation.

The Device owns:

- Configuration
- Sensors
- Actuators
- Diagnostics

The Device coordinates the complete EnvNode.

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
- presentation-unit configuration

Configuration determines how the Device behaves and how Measurements are presented externally.

Configuration is never considered Measurement data.

Examples of valid Configuration include:

- rain gauge calibration
- solar sensor calibration factor
- rain detector threshold
- heater limits
- MQTT server
- WiFi credentials
- timezone
- Temperature presentation unit
- AtmosphericPressure presentation unit

Presentation-unit configuration is global per MeasurementType within one Device.

It affects only external representation.

It never changes:

- Sensor acquisition
- canonical Measurement values
- Measurement validity
- calibration
- physical meaning

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

- atmospheric pressure
- temperature

SHT4x

- temperature
- relative humidity

Solar Sensor

- solar irradiance
- solar cell temperature

Rain Detector

- detector level
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

The Sensor is responsible for obtaining reliable physical values from its measurement source.

A Sensor may perform calculations that are technically required to derive a meaningful physical Measurement.

Examples include:

- ADC conversion
- calibration
- temperature compensation
- signal filtering
- debounce
- hardware oversampling
- conversion from raw hardware representation into engineering units

These operations belong to measurement acquisition.

Every Sensor must convert its hardware-specific representation into the canonical representation defined by the corresponding MeasurementType before emitting Measurement content.

Different Sensors producing the same MeasurementType may use completely different hardware representations.

For example:

    SHT4x
        |
        | hardware-native temperature
        v
    canonical Temperature in °C

and:

    BMP390
        |
        | hardware-native temperature
        v
    canonical Temperature in °C

Higher domain layers therefore never need to understand the native representation of individual Sensors.

A Sensor must never apply user-selected Presentation Units.

Presentation conversion belongs outside Sensor acquisition.

A Sensor must never perform weather interpretation.

Examples of allowed Sensor behaviour:

    raw ADC value
        |
        v
    calibrated and normalized RainDetectorLevel

and:

    rain gauge switch transition
        |
        v
    debounced RainGaugeTip event

Examples of behaviour that does not belong to a Sensor:

- rainfall today
- sunshine duration
- pressure trend
- evapotranspiration
- weather classification

A physical Sensor and a simulated Sensor must expose the same functional behaviour to the rest of the domain.

---

# Measurement

A Measurement represents one observed physical value or one physical event.

Measurements are the primary output of Sensors.

A Measurement is independent from the concrete hardware implementation that produced it.

Examples include:

- temperature
- relative humidity
- atmospheric pressure
- solar irradiance
- solar cell temperature
- rain detector level
- rain detector wet/dry state
- rain gauge tip event

A Measurement conceptually contains:

- Measurement type
- source Sensor
- timestamp
- value or event
- validity
- quality
- provenance

Not every Measurement requires every property.

A temperature Measurement contains a numeric value.

A rain gauge tip Measurement represents an event and therefore contains no artificial numeric value.

Every valid numeric Measurement uses the canonical representation defined by its MeasurementType.

The canonical representation is part of the MeasurementType semantics.

It is not selected independently by each Sensor and is not stored as an arbitrary unit field in every Measurement.

Examples:

    Temperature
        canonical representation: °C

    RelativeHumidity
        canonical representation: %

    AtmosphericPressure
        canonical representation: Pa

    SolarIrradiance
        canonical representation: W/m²

Presentation preferences do not change the Measurement itself.

For example, a canonical Temperature Measurement may later be presented externally as:

- degrees Celsius
- degrees Fahrenheit

without modifying the underlying Measurement.

The purpose of Measurement is to describe physical reality in a normalized domain representation.

Consumers of canonical Measurements must not need to understand:

- GPIO pins
- I2C addresses
- ADC channels
- register layouts
- hardware-specific scaling
- native Sensor units
- user-selected Presentation Units
- whether the source is simulated or physical

Measurements may represent either a continuous physical quantity or a discrete physical event.

Examples of continuous Measurements include:

- temperature
- relative humidity
- atmospheric pressure
- solar irradiance

Examples of event Measurements include:

- rain gauge tip

Both are Measurements.

Operational events such as Sensor failure or Sensor recovery are Diagnostics, not Measurements.

---

# Measurement Type

MeasurementType identifies what a Measurement represents.

Examples include:

- Temperature
- RelativeHumidity
- AtmosphericPressure
- SolarIrradiance
- SolarCellTemperature
- RainDetectorLevel
- RainDetectorWet
- RainGaugeTip

MeasurementType is independent from Sensor identity.

This distinction is important because different Sensors may produce the same MeasurementType.

Example:

    SHT4x
        |
        +----> Temperature

    BMP390
        |
        +----> Temperature

Both Measurements represent Temperature.

They remain distinguishable through their source Sensor.

MeasurementType therefore answers:

> What was measured?

Sensor identity answers:

> Where did the Measurement come from?

Each MeasurementType defines exactly one canonical representation.

The current canonical representations are:

| MeasurementType | Canonical Representation |
|---|---|
| Temperature | degrees Celsius |
| RelativeHumidity | percent, 0 to 100 |
| AtmosphericPressure | Pascal |
| SolarIrradiance | watt per square metre |
| SolarCellTemperature | degrees Celsius |
| RainDetectorLevel | normalized ratio, 0.0 to 1.0 |
| RainDetectorWet | Boolean, no unit |
| RainGaugeTip | event, no value and no unit |

Canonical representation is part of the domain contract.

All Sensors producing the same MeasurementType must emit the same canonical representation.

Presentation Units are separate from MeasurementType's canonical representation.

A MeasurementType may support one or more Presentation Units for external representation.

Examples:

Temperature

- degrees Celsius
- degrees Fahrenheit

AtmosphericPressure

- Pascal
- hectopascal
- kilopascal
- inches of mercury

Presentation-unit selection does not alter MeasurementType semantics or the canonical Measurement value.

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

Periodic acquisition scheduling is orchestration state owned by SensorManager. Hardware-near filtering, smoothing, debounce, oversampling and compensation remain Sensor acquisition responsibilities and use Sensor-specific configuration.

When SensorManager accepts Measurement content, it always copies the Sensor identity from `sensor.id()`, always assigns provenance from `sensor.provenance()`, validates structural Measurement Type and value-kind compatibility, and assigns a synchronized Unix Epoch timestamp.

SensorManager does not own hardware-specific range validation, conversion or physical plausibility rules.

Those remain Sensor responsibilities.

The primary purpose of the Device is to acquire Measurements rather than permanently storing them.

Long-term storage, aggregation and historical analysis belong outside the EnvNode domain.

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

The purpose of simulation is to validate the complete EnvNode domain model before physical hardware is available.

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

The EnvNode domain ends with:

- canonical Measurements
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

Presentation Unit selection belongs to Device Configuration because it expresses an installation-wide preference.

Presentation conversion itself occurs at the external representation boundary and does not change the canonical domain Measurement.

The domain describes what the EnvNode measures and how those Measurements are represented canonically.

The Technical Architecture describes how canonical Measurements are converted, serialized and transported externally.

---

# Core Rule

The central rule of the EnvNode domain is:

Sensors produce Measurements.

Measurements describe physical reality.

Measurements intentionally contain no weather interpretation.

Sensors convert hardware-specific representations into canonical Measurements.

Canonical Measurements remain stable inside the domain.

Presentation preferences may change external representation but never physical meaning.

Interpretation belongs outside the Device.

Examples belonging to the domain:

- temperature
- relative humidity
- atmospheric pressure
- solar irradiance
- rain gauge tip
- rain detected

Examples outside the domain:

- rainfall today
- sunshine duration
- pressure trend
- evapotranspiration
- irrigation requirement

The Device measures.

External systems understand.

This distinction is fundamental to the EnvNode architecture.
