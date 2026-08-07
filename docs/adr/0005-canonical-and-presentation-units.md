# ADR-0005

# Canonical and Presentation Units

- Status: Accepted
- Date: 2026-08-07

---

## Context

WeatherStation acquires physical values from Sensors with different hardware interfaces and native representations.

Examples include:

- digital Sensor register values
- ADC counts
- calibrated electrical signals
- values already expressed in engineering units by a Sensor

External consumers may expect Measurements in units that differ from the stable internal representation.

For example, atmospheric pressure may be acquired in a Sensor-specific representation, stored canonically in Pascal and presented externally in hectopascal.

Without an explicit unit boundary, hardware conversion, domain representation and user-facing presentation could become mixed across Sensors, SensorManager, MeasurementPublisher and MQTT transport.

That would make Measurements dependent on installation preferences and could cause two Sensors producing the same MeasurementType to use different internal representations.

---

## Decision

WeatherStation defines three distinct unit layers.

### Hardware Representation

Hardware Representation is the native form in which a Sensor obtains data.

Examples include:

- ADC counts
- register values
- voltage
- resistance
- Sensor-specific fixed-point values

Hardware Representation is private to the Sensor and its driver.
Different Sensors producing the same MeasurementType may use completely different Hardware Representations.

It is never published as the primary physical Measurement and never enters the Measurement pipeline as an arbitrary unit.

Raw hardware values may be exposed separately through Diagnostics when useful.

### Canonical Measurement Unit

Each MeasurementType has exactly one canonical domain representation.

Sensors convert Hardware Representation into the canonical unit before emitting Measurement content.

Every valid Measurement contains values expressed in the canonical representation defined by its MeasurementType.

Canonical units are stable and independent from:

- Sensor hardware
- Sensor identity
- installation preferences
- MQTT representation
- user interface preferences

The canonical representations are:

| MeasurementType | Canonical Representation |
|-----------------|--------------------------|
| Temperature | degrees Celsius |
| RelativeHumidity | percent, 0 to 100 |
| AtmosphericPressure | Pascal |
| SolarIrradiance | watt per square metre |
| SolarCellTemperature | degrees Celsius |
| RainDetectorLevel | normalized ratio, 0.0 to 1.0 |
| RainDetectorWet | boolean, no unit |
| RainGaugeTip | event, no unit |

Unknown has no physical value and no unit.

### Presentation Unit

Presentation Unit is the configured representation used when a canonical Measurement is exposed outside the domain.

MeasurementPublisher converts canonical numeric values into the selected Presentation Unit while preparing the external representation.

Presentation conversion never mutates the Measurement and never changes its timestamp, validity, quality, source or provenance.

The initial supported Presentation Units are:

| MeasurementType | Supported Presentation Units |
|-----------------|------------------------------|
| Temperature | degrees Celsius, degrees Fahrenheit |
| RelativeHumidity | percent |
| AtmosphericPressure | Pascal, hectopascal, kilopascal, inches of mercury |
| SolarIrradiance | watt per square metre |
| SolarCellTemperature | degrees Celsius, degrees Fahrenheit |
| RainDetectorLevel | normalized ratio, percent |
| RainDetectorWet | boolean, no unit |
| RainGaugeTip | event, no unit |

The canonical unit is the default Presentation Unit whenever no alternative is configured.

Invalid or unsupported presentation configuration falls back to the canonical representation rather than changing Sensor behaviour or invalidating Measurements.

Externally presented numeric values include unit metadata so consumers can interpret the representation unambiguously.

Value-free events and boolean state Measurements do not acquire artificial units.

---

## Presentation Configuration

Presentation Unit selection is global per MeasurementType within one Device.

All Sensors producing the same MeasurementType therefore use the same external Presentation Unit.

For example, temperature from SHT4x and temperature from BMP390 are both presented using the globally selected Temperature unit.

Presentation configuration is not stored per Measurement and is not selected independently per Sensor.

ConfigurationService remains the authoritative owner of persistent presentation configuration.

MeasurementPublisher obtains presentation configuration through ConfigurationService and does not persist presentation settings itself.

Changing Presentation Unit configuration affects future external representations only.

It does not modify:

- Sensor acquisition
- canonical Measurement values
- previously published Measurements
- Measurement timestamps
- Sensor calibration

---

## Responsibilities

### Sensor

The Sensor owns:

- reading Hardware Representation
- hardware-specific conversion
- calibration
- compensation
- physical plausibility validation
- conversion into the canonical Measurement unit

The Sensor never applies Presentation Unit preferences.

### SensorManager

SensorManager accepts and validates canonical Measurement content.

It assigns source, synchronized timestamp and provenance as defined by ADR-0004.

SensorManager never converts units and never reads Presentation Unit configuration.

### MeasurementPublisher

MeasurementPublisher owns:

- selecting the configured Presentation Unit for the MeasurementType
- converting canonical numeric values into that Presentation Unit
- including unambiguous unit metadata in the external representation
- serializing the presented Measurement

MeasurementPublisher does not mutate the canonical Measurement.

Presentation conversion is purely representational. The physical meaning of a Measurement never changes.

It performs representation conversion only and does not interpret weather data.

### MqttService

MqttService transports the topic and payload prepared by MeasurementPublisher.

MqttService has no knowledge of:

- canonical units
- presentation units
- unit conversion
- MeasurementType semantics

---

## Relationship to Time and the Measurement Pipeline

ADR-0003 remains authoritative for time representation.

Unit conversion does not alter the Measurement epoch. MeasurementPublisher formats the assigned epoch independently from presentation-unit conversion.

ADR-0004 remains authoritative for the Measurement pipeline.

Canonical Measurements follow the existing pipeline:

    Sensor
        |
        | canonical Measurement content
        v
    SensorManager
        |
        | completed canonical Measurement
        v
    MeasurementPublisher
        |
        | presentation conversion and serialization
        v
    MqttService

Simulation follows exactly the same canonical and presentation-unit rules as physical hardware.

---

## Rationale

One canonical unit per MeasurementType provides:

- deterministic domain semantics
- consistent values from different Sensors
- simpler structural validation
- hardware independence
- presentation independence
- a stable Measurement pipeline

Applying presentation conversion in MeasurementPublisher provides:

- one conversion point
- consistent output across all Sensors
- configurable external representation
- no presentation dependencies in Sensors or SensorManager
- no Measurement semantics in MqttService

Global configuration per MeasurementType avoids contradictory units for equivalent physical quantities within one Device.

---

## Consequences

Sensors must emit values in canonical units even if the hardware provides another representation.

Measurement values remain comparable within the Device regardless of their source Sensor.

Presentation preferences do not affect acquisition, calibration or physical validation.

MeasurementPublisher requires access to validated presentation configuration.

Published numeric values identify their Presentation Unit explicitly.

Adding a new MeasurementType requires defining:

- its canonical representation
- its supported Presentation Units
- its default Presentation Unit

Adding a new Presentation Unit affects representation conversion only.

Adding a new Presentation Unit never requires changes to Sensor implementations.

Changing presentation configuration does not require Sensor reinitialization.

No unit conversion is performed for invalid Measurements, boolean state Measurements or value-free events.

---

## Alternatives Considered

### Store an arbitrary unit in every Measurement

Rejected.

This would allow equivalent MeasurementTypes to carry inconsistent internal representations and would make every consumer responsible for unit handling.

---

### Apply presentation conversion inside each Sensor

Rejected.

This couples hardware acquisition to user preferences and duplicates conversion logic across physical and simulated Sensors.

---

### Configure presentation units per Sensor

Rejected.

Two Sensors producing the same MeasurementType could expose incompatible units from one Device.

---

### Convert units inside SensorManager

Rejected.

SensorManager owns orchestration and Measurement acceptance, not external representation.
Presentation conversion belongs at the external representation boundary.

---

### Convert units inside MqttService

Rejected.

MqttService owns broker connectivity and transport only. Unit conversion would introduce Measurement semantics into the transport layer.

---

### Publish canonical units only

Rejected as the permanent policy.

Canonical-only publication is simple but does not support installation-wide presentation preferences. Canonical units remain the fallback when no alternative is configured.

---

### Require every external consumer to convert units

Rejected.

This prevents the Device from providing one consistent configured presentation and duplicates conversion rules across consumers.

---

## Design Rule

Sensors normalize hardware values.

Measurements remain canonical.

SensorManager preserves canonical meaning.

MeasurementPublisher presents configured units.

MqttService transports without interpretation.
