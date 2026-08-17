# ADR-0004

# Measurement Pipeline

- Status: Accepted
- Date: 2026-08-07

---

## Context

WeatherStation acquires Measurements from physical and simulated Sensors.

Those Measurements are eventually published via MQTT.

Without a clearly defined pipeline, Sensor implementations would become tightly coupled to communication protocols.

This would reduce modularity and make simulation significantly more difficult.

---

## Decision

WeatherStation defines one common Measurement pipeline.

    Sensor
        |
        v
    Measurement
        |
        v
    SensorManager
        |
        v
    MeasurementPublisher
        |
        v
    MqttService
        |
        v
    MQTT Broker

All Measurements follow this pipeline.

Simulation uses exactly the same pipeline.

The acceptance boundary is refined as follows:

    Sensor
        |
        | emits Measurement content
        v
    SensorManager
        |
        | validates structural type/value compatibility
        | assigns source SensorId
        | assigns synchronized Unix Epoch timestamp
        | assigns Sensor provenance
        v
    Completed Measurement
        |
        v
    MeasurementPublisher
        |
        v
    MqttService

Sensors provide physical content, validity and quality.

SensorManager completes Measurements without modifying their physical meaning.

For each Sensor operation, SensorManager establishes a synchronous acceptance context containing the active Sensor and one shared timestamp. It always assigns source from `sensor.id()` and provenance from `sensor.provenance()`.

If system time is not synchronized, emitted content is discarded without forwarding or historical buffering.

It validates only structural consistency, such as:

- MeasurementType ↔ value representation
- valid SensorId
- synchronized timestamp availability

It never performs hardware-specific conversion, calibration or weather interpretation.
Sensor identity belongs to the Sensor. SensorManager copies that identity into the accepted Measurement.

SensorManager does not own hardware-specific conversion, range validation or physical plausibility rules.

Those remain Sensor responsibilities.

---

## Rationale

The pipeline separates domain concepts from communication.

Responsibilities become clearly defined.

Sensor

- hardware interaction
Measurement

- normalized domain representation
- independent of concrete hardware

SensorManager

- sensor orchestration
- periodic scheduling
- event handling
- structural validation
- source, timestamp and provenance assignment

MeasurementPublisher

- serialization
- topic generation

MqttService

- broker connectivity
- transport

One coherent Sensor acquisition may produce multiple Measurements.

Periodic and event-driven Sensors emit through the same output boundary.

Emission through that boundary is synchronous. The receiver consumes or copies each Measurement during the call and never retains the passed reference.

Sensor operation results have explicit emission semantics:

- Completed indicates no hardware or acquisition failure; `sample()` should normally have emitted at least one Measurement.
- NoData indicates normal completion with zero emitted Measurements.
- HardwareFailure indicates an operational failure and permits zero or more Measurements to have been emitted successfully first.

Successfully emitted Measurements are not rolled back. Partial output from a multi-output acquisition is allowed, and one Sensor operation is intentionally not transactional.

Event Measurements such as a rain gauge tip have no value.

Artificial numeric event values are not permitted.

Sensor failure, sensor recovery and hardware communication failure are Diagnostics rather than Measurement Types.

---

## Consequences

Sensor implementations never publish MQTT directly.

MeasurementPublisher becomes the single translation point between domain objects and MQTT.

Changing MQTT payloads or topic structures does not affect Sensor implementations.

Replacing a simulated Sensor with a physical implementation requires no architectural changes.

Sensor acquisition and local operation may continue before time synchronization.

Measurements are not forwarded into the publication pipeline with fake epoch timestamps. Pre-synchronization Measurements may be dropped rather than queued indefinitely.

---

## Alternatives Considered

### Sensors publish MQTT directly

Rejected.

This couples hardware drivers to communication protocols.

Simulation and testing become unnecessarily difficult.

---

### MQTT client inside every Sensor

Rejected.

This duplicates communication logic and violates separation of responsibilities.

---

## Design Rule

Sensors produce Measurements.

Measurements describe physical reality.

Diagnostics describe operational health.

SensorManager validates and completes Measurements without interpreting them.

MeasurementPublisher prepares communication.

MqttService performs communication.

## Implementation evolution (2026-08-17)

The accepted-Measurement boundary now fans out synchronously to both `MeasurementPublisher` and bounded `MeasurementSnapshotCache`. The cache stores only the latest copied snapshot per `SensorId + MeasurementType`, including monotonic acceptance time and revision identity. `IMeasurementResolver` exposes those copies to Measurement-driven Controllers without changing MQTT's role or introducing history storage or an event bus.
