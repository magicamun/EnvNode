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

---

## Rationale

The pipeline separates domain concepts from communication.

Responsibilities become clearly defined.

Sensor

- hardware interaction

Measurement

- domain representation

SensorManager

- orchestration

MeasurementPublisher

- serialization
- topic generation

MqttService

- broker connectivity
- transport

---

## Consequences

Sensor implementations never publish MQTT directly.

MeasurementPublisher becomes the single translation point between domain objects and MQTT.

Changing MQTT payloads or topic structures does not affect Sensor implementations.

Replacing a simulated Sensor with a physical implementation requires no architectural changes.

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

MeasurementPublisher prepares communication.

MqttService performs communication.