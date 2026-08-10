# ADR-0010

# Sensor Identity and External Measurement Addressing

- Status: Accepted
- Date: 2026-08-08

---

## Context

The original WeatherStation MQTT representation published one topic per MeasurementType.

Example:

    weatherstation/<device>/measurement/temperature

This model assumes that one Device produces at most one Measurement for each MeasurementType.

This assumption was valid during the initial simulation phase.

The first physical sensor integration (AM2302) demonstrated that this assumption no longer holds.

The WeatherStation may simultaneously contain multiple Sensors producing the same MeasurementType.

Examples include:

- simulated temperature
- AM2302 temperature
- SHT4x temperature
- BMP390 temperature

All of these are valid Measurements with different physical origins.

Using only MeasurementType in the external address causes later publications to overwrite earlier ones.

The domain model already distinguishes these Measurement sources through SensorId.

The external representation must preserve that distinction.

---

## Decision

External Measurement addressing shall identify Measurements by:

- Device
- Sensor
- MeasurementType

Conceptually:

    Device
        |
        v
    SensorId
        |
        v
    MeasurementType

The initial MQTT representation therefore becomes:

    envnode/<device>/sensor/<sensorId>/<measurementType>

Examples:

    envnode/WeatherStation/sensor/4/temperature

    envnode/WeatherStation/sensor/4/relativehumidity

    envnode/WeatherStation/sensor/3/temperature

    envnode/WeatherStation/sensor/3/atmosphericpressure

MeasurementType alone is no longer considered sufficient as an external address.

---

## Sensor Identity

Every logical Sensor within one Device owns one stable SensorId.

SensorId uniquely identifies the Measurement source within that Device.

SensorId remains stable across:

- reboot
- configuration reload
- firmware update

SensorId is assigned by the configured Sensor Slot.

The runtime Sensor therefore inherits the identity of its Slot.

No additional runtime-generated SensorId exists.

Conceptually:

    Sensor Slot 4

        |
        v

    Physical Sensor

        |
        v

    SensorId = 4

SensorId is therefore both:

- Slot identity
- runtime Sensor identity

---

## Sensor Slot

Sensor Slots are configuration objects.

A Slot defines:

- SensorId
- enabled state
- implementation
- configuration
- user-defined name

The Slot itself is not a Measurement source.

The runtime Sensor created from the Slot produces Measurements using the Slot's SensorId.

---

## Sensor Name

Each Sensor Slot owns one configurable user-visible name.

Examples:

- Outside
- Greenhouse
- Roof
- Rain Plate

The Sensor Name exists exclusively for human interaction.

Typical uses include:

- Web Interface
- Diagnostics
- logging
- Home Assistant metadata

Changing the Sensor Name does not change:

- SensorId
- MQTT topic
- Measurement identity

Sensor Name is therefore presentation metadata rather than technical identity.

---

## Sensor Type

Sensor Type is derived from the Sensor implementation.

Examples include:

- AM2302
- SHT4x
- BMP390
- SimulatedTemperature

Sensor Type is not user-configurable.

The implementation determines the Type automatically.

Sensor Type is intended for:

- diagnostics
- Web Interface
- inventory
- development

Sensor Type is not part of Measurement identity.

---

## External Addressing

External addressing shall remain stable.

Therefore external addresses never contain:

- Sensor Name
- implementation name
- hardware model
- GPIO number

Only SensorId identifies the Measurement source.

This guarantees stable addresses even if:

- a Sensor is renamed
- implementation changes
- hardware is replaced

Example:

Slot 4 initially:

    AM2302

later replaced by:

    SHT4x

The external address remains:

    envnode/<device>/sensor/4/temperature

Consumers therefore do not require reconfiguration after hardware replacement.

---

## Measurement Payload

Measurement payloads contain Measurement data.

Measurement identity is provided by the MQTT topic.

Payloads therefore do not require duplicated identity information in order to distinguish different Sensors.

Additional metadata such as:

- Sensor Name
- Sensor Type

may be published separately if required.

They are not part of Measurement identity.

---

## Responsibilities

### Sensor

Sensor owns:

- physical acquisition
- canonical Measurement creation
- Sensor Type

Sensor does not own:

- SensorId assignment
- external addressing
- MQTT topics

---

### Sensor Slot

Sensor Slot owns:

- SensorId
- configuration
- user-visible name
- implementation selection

---

### SensorManager

SensorManager owns:

- runtime Sensor registration
- SensorId propagation
- timestamp assignment
- provenance assignment

SensorManager does not construct external addresses.

---

### MeasurementPublisher

MeasurementPublisher owns:

- external Measurement addressing
- topic generation
- payload serialization

MeasurementPublisher derives the topic using:

- Device
- SensorId
- MeasurementType

---

### MqttService

MqttService transports the generated topic and payload.

MqttService has no knowledge of:

- Sensor Slots
- Sensor Names
- Measurement identity

---

## Rationale

Addressing Measurements by SensorId provides:

- unique Measurement addresses
- simultaneous Sensors of the same MeasurementType
- stable MQTT subscriptions
- hardware independence
- configuration independence

Separating SensorId, Name and Type prevents presentation concerns from becoming part of technical identity.

Replacing hardware therefore does not invalidate external integrations.

---

## Consequences

Multiple Sensors may simultaneously publish identical MeasurementTypes.

Each Sensor owns an independent Measurement stream.

Changing Sensor Name has no external protocol impact.

Replacing one Sensor implementation with another preserves MQTT topics.

MeasurementPublisher requires SensorId when constructing external addresses.

Future communication protocols should preserve the same addressing model independently of MQTT.

---

## Alternatives Considered

### Address Measurements only by MeasurementType

Rejected.

Multiple Sensors overwrite each other.

---

### Address Measurements by Sensor Name

Rejected.

Sensor Names are user-configurable and therefore unstable.

---

### Address Measurements by Sensor Type

Rejected.

Replacing hardware would change external addresses.

---

### Generate random runtime Sensor identifiers

Rejected.

Runtime identities would not survive reboot or configuration changes.

---

### Maintain independent SlotId and SensorId

Rejected.

The runtime Sensor directly represents its configured Slot.

Maintaining two separate identities would duplicate information without providing additional value.

---

## Design Rule

Sensor Slots own configuration.

SensorId provides stable technical identity.

Sensor Names serve human interaction.

Sensor Type identifies the implementation.

MeasurementPublisher exposes:

    Device
        →
    SensorId
        →
    MeasurementType

as the authoritative external Measurement address.
