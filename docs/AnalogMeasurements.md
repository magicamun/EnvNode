# Analog Measurements in EnvNode

## Status and Purpose

This document describes the planned architecture for analog measurements in EnvNode. It records the responsibility boundaries, data flow, and deliberately limited scope of the first implementation.

The first real-world use case is a hydrostatic cistern probe with a 4–20 mA output. Its current signal is converted to a voltage and read by an ADC. The architecture must solve this case cleanly while allowing the ADC to be replaced and individual components to be reused later.

The C++ structures shown here are pseudocode. Names and details may be adapted to existing EnvNode conventions during implementation, provided that the responsibility boundaries described here remain intact.

## Motivation

An analog measurement consists of several conceptually different steps:

- A physical sensor produces an electrical signal.
- An ADC captures that signal.
- Individual readings are scheduled and filtered.
- The electrical signal is converted into a physical quantity.
- One or more publishable measurements are produced.
- Additional measurements may be derived from those measurements and further domain knowledge.

Combining all these responsibilities in a single class would unnecessarily couple sensor logic to a particular ADC, filter, tank, or publication schedule. An ADC should not know anything about cisterns, and a pressure sensor should not know the geometry of a tank.

The architecture therefore separates the following layers:

```text
Physical Sensor
      │ electrical signal
      ▼
IAnalogInput
      │ electrical quantity, preferably volts
      ▼
Sampling / Filtering
      │ stabilized electrical value
      ▼
AnalogSensor (V1: AnalogPressureSensor)
      │ physical measurement(s)
      ▼
Measurement
      │
      ▼
Derived Measurement(s)
```

This separation is a conceptual model. It does not necessarily require a separate runtime class for every arrow.

## Terms and Responsibilities

### Physical Sensor

The physical sensor is the actual probe, including its electrical characteristics—for example, a hydrostatic 4–20 mA pressure probe. It exists outside the software but determines the calibration and validity boundaries.

### `IAnalogInput`

`IAnalogInput` abstracts electrical acquisition. It encapsulates ADC-specific details such as the channel, attenuation or gain, reference, resolution, I²C address, and conversion of the raw ADC value where necessary.

Its output is preferably a voltage in volts. A common electrical unit keeps downstream components independent of the resolution and raw-value format of a particular ADC.

`IAnalogInput` explicitly has no sensor semantics. Concepts such as pressure, water level, cistern, or liters do not belong in this layer.

Planned backends include at least:

- `Esp32AnalogInput` for the ESP32 internal ADC
- `Ads1115AnalogInput` for an external ADS1115

### Sampling and Filtering

This layer determines when individual readings are taken and how they are combined into a stabilized value. Sampling must be time-controlled and therefore independent of the incidental execution speed of the main loop.

An arithmetic mean (`Average`) is sufficient for V1. Possible later additions are `Median` and `ExponentialMovingAverage` (`EMA`). The design should allow these additions without introducing a comprehensive filtering framework prematurely.

### `AnalogSensor`

An analog sensor interprets the stabilized electrical quantity in its physical context. V1 deliberately does not introduce a universal, overly generic `AnalogSensor` implementation. Instead, the first real sensor will be a small `AnalogPressureSensor`.

It applies validity checks and two-point calibration and produces:

- `pressure`
- optionally `water_level`, if the required conversion is configured

### `Measurement`

A `Measurement` is a named physical quantity with a value, unit, timestamp, and quality information. It is the result that EnvNode processes or publishes.

### Derived Measurement

A derived measurement is calculated from an existing measurement and additional domain knowledge. Tank volume is therefore not a responsibility of `AnalogPressureSensor`; it is derived from `water_level` and the tank geometry.

A geometric formula is sufficient for simple, symmetrical tanks. A `height -> volume` characteristic curve should be supported later for asymmetrical tanks.

## Data Flow

### General Flow

```text
┌──────────────────────┐
│ Physical Sensor      │
│ e.g. 4–20 mA         │
└──────────┬───────────┘
           │
           ▼
┌──────────────────────┐
│ Signal Conditioning  │
│ current → voltage    │
└──────────┬───────────┘
           │ voltage
           ▼
┌──────────────────────┐
│ IAnalogInput         │
│ ESP32 or ADS1115     │
└──────────┬───────────┘
           │ AnalogSample in volts
           ▼
┌──────────────────────┐
│ Sampling / Filtering │
│ V1: Average          │
└──────────┬───────────┘
           │ FilteredAnalogValue
           ▼
┌──────────────────────┐
│ AnalogPressureSensor │
│ validation +         │
│ calibration          │
└───────┬────────┬─────┘
        │        │
        ▼        ▼
   pressure   water_level (optional)
                    │
                    ▼
            ┌───────────────────┐
            │ TankVolumeDerived │
            │ Measurement       │
            └─────────┬─────────┘
                      ▼
                    volume
```

### Separate Time Intervals

Sampling, filter calculation, and publication are different operations and are configured independently:

```text
Time ─────────────────────────────────────────────────────────►

Sampling:       •  •  •  •  •  •  •  •  •  •  •  •
                   sampleInterval

Filter windows: [────────────][────────────][────────────]
                        filterWindow

Measurement /
Publish:        ▲                         ▲
                └──── measurementInterval ┘
```

- `sampleInterval` determines the frequency of individual ADC reads.
- `filterWindow` determines which samples are evaluated together.
- `measurementInterval` determines when a new domain-level measurement is produced.
- If EnvNode schedules measurement creation and transport separately, an additional `publishInterval` may determine transmission. It must not implicitly control sampling.

For example, the input may be read every 20 ms, averaged over one second, and used to produce or publish a measurement only every 60 seconds.

## Proposed Interfaces and Configuration

### Electrical Input

```cpp
struct AnalogSample {
    float voltage;              // volts
    Timestamp capturedAt;
    bool valid;                 // technical read succeeded
    AnalogInputError error;     // None, NotInitialized, BusError, ...
};

class IAnalogInput {
public:
    virtual ~IAnalogInput() = default;

    virtual bool begin() = 0;
    virtual AnalogSample read() = 0;
    virtual bool initialized() const = 0;
};
```

At this layer, `valid` only indicates whether a technically usable electrical value was read. The domain sensor decides whether that value represents a connected and plausibly operating probe by applying its input boundaries.

Possible backend configurations:

```cpp
struct Esp32AnalogInputConfiguration {
    int gpio;
    AdcAttenuation attenuation;
    AdcResolution resolution;
};

struct Ads1115AnalogInputConfiguration {
    I2cBusId bus;
    uint8_t address;
    Ads1115Channel channel;
    Ads1115Gain gain;
};
```

ADC-specific configuration remains with the respective backend and does not become part of the pressure-sensor configuration.

### Sampling and Filtering

```cpp
enum class AnalogFilterType {
    Average,
    // optional later additions:
    Median,
    ExponentialMovingAverage
};

struct AnalogSamplingConfiguration {
    Duration sampleInterval;
    Duration filterWindow;
    AnalogFilterType filter = AnalogFilterType::Average;
};

struct FilteredAnalogValue {
    float voltage;              // volts
    Timestamp windowStart;
    Timestamp windowEnd;
    size_t acceptedSamples;
    size_t rejectedSamples;
    bool valid;
};
```

For the V1 `Average` filter:

```text
filteredVoltage = sum(valid sample voltages) / numberOfValidSamples
```

The minimum number of technically valid samples and the maximum permitted age of the last filtered value remain to be defined.

### Two-Point Calibration

Calibration linearly maps two electrical reference points to two physical reference points:

```cpp
struct LinearTwoPointCalibration {
    float inputAtMin;           // e.g. volts at 0 bar
    float valueAtMin;           // e.g. 0 bar
    float inputAtMax;           // e.g. volts at 0.5 bar
    float valueAtMax;           // e.g. 0.5 bar
};
```

For an input `x`, the result is:

```text
value = valueAtMin
      + (x - inputAtMin)
      * (valueAtMax - valueAtMin)
      / (inputAtMax - inputAtMin)
```

Configuration must be rejected at startup if `inputAtMin == inputAtMax`. An inverted characteristic can be valid and should not be prohibited without a domain-specific reason.

### Pressure Sensor

```cpp
struct AnalogPressureSensorConfiguration {
    // Electrical boundaries outside which a plausibly connected
    // sensor can no longer be assumed.
    float validInputMinVoltage;
    float validInputMaxVoltage;

    // Electrical-to-physical mapping.
    LinearTwoPointCalibration pressureCalibration;

    // Lower boundary for a reliable physical result.
    float minimumReliablePressure;

    Duration measurementInterval;

    // Optional pressure-to-water-level conversion, e.g. using
    // density and gravitational acceleration.
    optional<HydrostaticLevelConfiguration> waterLevel;
};

class AnalogPressureSensor {
public:
    AnalogPressureSensor(
        IFilteredAnalogValueSource& input,
        AnalogPressureSensorConfiguration configuration);

    vector<Measurement> measure();
};
```

The concrete names for units and measurement metadata will be adapted to the existing EnvNode model during implementation.

### Derived Tank Volume

```cpp
class ITankVolumeModel {
public:
    virtual ~ITankVolumeModel() = default;
    virtual float volumeForHeight(float height) const = 0;
};

struct CylindricalTankGeometry {
    float diameter;
    float usableHeight;
};

struct HeightVolumePoint {
    float height;
    float volume;
};

struct HeightVolumeCurve {
    vector<HeightVolumePoint> points; // interpolation to be defined later
};

class TankVolumeDerivedMeasurement {
public:
    Measurement derive(
        const Measurement& waterLevel,
        const ITankVolumeModel& geometry);
};
```

The derived measurement inherits or degrades the quality of its input measurement. It must never produce a valid volume from an invalid water level.

## Boundaries: Validity, Calibration, and Measurement Range

Three kinds of boundaries must not be conflated.

### `validInputMin` and `validInputMax`

These boundaries apply to the actual electrical input signal. A value outside this range means that no credible sensor signal can be assumed—for example, because of a disconnected probe, broken wire, short circuit, or ADC failure.

Such a value is **invalid**. It must not be extrapolated into an apparently normal pressure value.

### `calibrationMin` and `calibrationMax`

These are the two reference points that map the electrical input to the physical quantity. They define the calibration line, not necessarily the technical validity of the input.

Conceptually, these terms correspond to `inputAtMin`/`valueAtMin` and `inputAtMax`/`valueAtMax` in `LinearTwoPointCalibration`.

An input may therefore lie outside the calibration reference points while remaining inside the technically valid input range. Whether such a value is extrapolated, clamped, or marked as outside the measurement range is a separate domain decision.

### `minimumReliableRange`

This boundary describes the range above which the sensor produces a sufficiently reliable physical result. A connected sensor may produce a technically valid signal below this boundary even though the resulting quantity is below its specified measurement range.

For the pressure sensor, this is initially represented as `minimumReliablePressure`. A later generalization to `minimumReliableRange` must not conflate electrical and physical boundaries.

## Quality and Validity Semantics

Validity and quality express different facts:

- **Validity** answers: Is the measurement technically and logically usable?
- **Quality** answers: What limitation or classification applies to an otherwise usable value?

Proposed semantics:

| Situation | Validity | Quality | Value |
|---|---:|---|---|
| ADC or bus error, or no sample | invalid | `input_error` | no domain value |
| Input below `validInputMin` or above `validInputMax` | invalid | `invalid_input` | no extrapolated domain value |
| Probe connected, but value below its reliable range | valid | `below_measurement_range` | calibrated value or defined boundary value |
| Value within the reliable measurement range | valid | `good` | calibrated value |
| Derived measurement with an invalid source | invalid | `source_invalid` | no derived value |

For the reference use case, this specifically means:

```text
disconnected probe
    → electrical input outside validInputMin/Max
    → measurement invalid / quality invalid_input

connected probe, water level below the specified measurement range
    → electrical input valid
    → measurement valid / quality below_measurement_range
```

This distinction separates “no sensor value is available” from “the sensor reports a very small value that is not sufficiently accurate.”

It remains open whether `valid` should exist as a separate field or be derived entirely from an EnvNode-wide status model. The semantic distinction must be preserved either way.

## Reference Use Case: Hydrostatic Cistern Probe

The first concrete setup consists of:

```text
Water height
   ↓ hydrostatic pressure
4–20 mA pressure probe
   ↓ current
Current-to-voltage converter
   ↓ voltage
ESP32 ADC or ADS1115
   ↓ samples in volts
Average filter
   ↓ stabilized voltage
AnalogPressureSensor
   ├─ pressure
   └─ water_level (optional)
          ↓
TankVolumeDerivedMeasurement + tank geometry
          ↓
       volume
```

The current-to-voltage converter is initially considered part of the physical signal chain. Because `IAnalogInput` returns volts, the pressure calibration contains the voltage reference points actually observed by the ADC. V1 does not require a separate generic current-signal abstraction.

`AnalogPressureSensor` may produce `water_level` in addition to `pressure`, because the hydrostatic conversion is closely related to the interpretation of pressure. The necessary assumptions—such as liquid density and gravitational acceleration, or a simplified conversion factor—must be configured explicitly.

Tank volume remains separate. Two tanks may use the same pressure probe and report the same water level while containing different volumes because their geometries differ.

## Deliberate V1 Scope

V1 consists of small, replaceable components and one real use case:

- an `IAnalogInput`
- at least the ADC backend required first; the other backend follows using the same interface
- deterministic sampling
- an `Average` filter
- two-point calibration
- `AnalogPressureSensor`
- `pressure` and optional `water_level` measurements
- a tank-volume derived measurement separate from the pressure sensor
- explicit validity and quality handling

V1 does not include:

- a universal analog-sensor DSL
- arbitrary calibration curves for every sensor type
- a comprehensive interchangeable filtering framework
- automatic sensor detection
- a generic model for every type of electrical signal
- a characteristic curve for asymmetrical tanks, unless the first real tank immediately requires one

Only a second real analog sensor type should determine which parts truly need to be generalized.

## Open Design Questions

The first `AnalogPressureSensor` implementation resolves several V1 decisions:

- `AverageAnalogSampler` owns time-based acquisition windows and is serviced through the sensor's fast `service()` path.
- A completed window with too few valid samples produces invalid, degraded domain measurements but is still a completed sensor operation.
- Electrically valid inputs are linearly extrapolated outside the calibration reference points without clamping.
- Values below the reliable pressure boundary retain their calibrated value and use `below_measurement_range` quality.
- Each scheduled domain `sample()` publishes the latest completed window, allowing the measurement interval to remain slower than the filter interval.
- Optional water level is calculated directly by `AnalogPressureSensor`; tank volume remains a separate future derived measurement.

The remaining questions are deliberately outside this implementation:

1. **Value age:** Whether a maximum age should be imposed on the latest completed filter window.
2. **Derived measurements:** Which future dependency or transformation mechanism should own tank volume.
3. **Tank characteristic curve:** Which interpolation method and out-of-range behavior a future `height -> volume` curve should use.
4. **Diagnostic data:** Whether raw voltage, sample count, and rejected samples should be exposed as diagnostics.
5. **Configuration integration:** How configuration errors should be reported when this sensor is later added to persistence, factories, and the web UI.

## Concrete Next Implementation Steps

1. Review existing EnvNode interfaces for scheduling, configuration, `Measurement`, units, timestamps, and quality codes.
2. Define `IAnalogInput` and `AnalogSample`, using volts as their common output.
3. Implement the backend required first for the cistern setup (`Esp32AnalogInput` or `Ads1115AnalogInput`) and test it with known voltages.
4. Implement a small time-controlled sampler with an `Average` filter. Define the behavior for individual read failures, minimum sample count, and time windows through tests.
5. Implement two-point calibration as an independently testable function or value object.
6. Implement `AnalogPressureSensor` with separate input-validity, calibration, and reliability boundaries.
7. Add tests for all important boundary cases: values immediately below and above each boundary, a disconnected probe, an ADC failure, a value below the measurement range, and a normal reading.
8. Produce `pressure` and optional `water_level` with the same time basis and traceable quality propagation.
9. Implement tank volume as a separate derived measurement, initially for the actual tank geometry.
10. Validate the complete chain on the real cistern setup and document observed raw voltages, noise, suitable sample intervals, and filter windows.
11. Implement the second ADC backend against the same `IAnalogInput` interface to confirm interchangeability in practice.
12. Only after adding another analog sensor type, evaluate which components should be generalized and whether Median, EMA, or a `height -> volume` characteristic curve is actually required.

## Architecture Decisions at a Glance

- The ADC provides an electrical quantity, preferably volts, and has no sensor semantics.
- ESP32 and ADS1115 are interchangeable backends behind `IAnalogInput`.
- Sampling, filtering, measurement, and publication intervals are independent.
- V1 uses `Average`; Median and EMA remain optional future additions.
- Physical interpretation uses two-point calibration.
- Electrical validity boundaries, calibration reference points, and the reliable measurement range are separate concepts.
- A disconnected probe is invalid; a connected sensor below its measurement range remains valid with quality `below_measurement_range`.
- The first concrete sensor is `AnalogPressureSensor`, not a universal analog-sensor architecture.
- The pressure sensor produces `pressure` and optionally `water_level`.
- Tank volume is a derived measurement based on `water_level` and tank geometry.
- Asymmetrical tanks may later be represented by a `height -> volume` characteristic curve.
