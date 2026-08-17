# ADR-0013: Capability-based Actuator and Controller runtimes

## Status

Accepted

## Date

2026-08-17

## Implementation status

Controller infrastructure v1 is implemented, tested and physically verified for Blink and Threshold/Hysteresis behavior.

Actuator and Controller external self-description schema 1 is implemented through a separate MQTT
publication adapter. Pure builders translate persisted slot configuration plus registry metadata
into protocol documents; the retained publisher reconciles all fixed slots on connection and
configuration changes. Runtime status remains a separate concern.

## Context

EnvNode needs local physical-output control without coupling Web, MQTT or local coordination behavior to GPIO drivers. Runtime composition must support configured instances, deterministic embedded storage and live replacement. Controllers must remain operational across Actuator replacement without retaining stale pointers.

Persistent Controller composition must also remain distinct from transient runtime Start/Stop state. MQTT needs to expose both authoritative parameter state and parameter mutation without interpreting retained state publications as commands.

## Decision

Sensors, Actuators and Controllers remain separate domain and runtime models.

Actuators use:

```text
ActuatorSlotConfiguration
 -> ActuatorImplementationRegistry
 -> ActuatorFactory
 -> ActuatorRuntime
 -> capability interface
 -> concrete implementation
```

`ActuatorId` identifies a configured instance. The first implementation is `gpio_on_off`, exposing `IOnOffActuator`. Hardware assignment belongs to the Actuator Slot; external adapters and Controllers address identity and capability, not GPIO.

Controllers use:

```text
ControllerSlotConfiguration
 -> ControllerImplementationRegistry
 -> ControllerFactory
 -> ControllerRuntime
 -> IController
 -> concrete implementation
```

The implemented Controller types are Blink and Threshold. Both retain target `ActuatorId` and resolve the current `IOnOffActuator` through `IOnOffActuatorResolver` and `ActuatorRuntime`; neither retains a concrete Actuator pointer across runtime replacement.

Threshold identifies one input stream with `MeasurementSourceReference` (`SensorId + MeasurementType`). It reads copied latest snapshots through `IMeasurementResolver`; it retains no Sensor pointer. `MeasurementSnapshotCache` supplies bounded latest-value storage, monotonic acceptance time for freshness and revision identity for change processing.

Both runtimes use bounded deterministic construction and live composition rebuild. Actuator shutdown requests a safe Off state where possible. Enabled Controllers start when a composition is activated; manual Start/Stop is transient and does not alter persistent `enabled`.

Web and MQTT are adapters over the same runtime and configuration boundaries. Controller MQTT uses separate topic categories:

- `status`: retained runtime truth
- `cmd`: transient START/STOP
- `parameter/<name>`: retained authoritative persistent value
- `cmd/parameter/<name>`: external persistent mutation request

EnvNode subscribes only to command topics. This prevents retained parameter state from feeding back into configuration mutation and runtime rebuild.

Exactly one configured enabled Controller may target an Actuator. Runtime STOP does not release that persisted ownership. This prevents Controller-versus-Controller contention without adding runtime arbitration. Manual Web or MQTT Actuator commands remain allowed and currently use last-command-wins semantics relative to the owning Controller.

## Evolution

The original implementation allowed multiple Controllers to target one Actuator and treated all command contention as last-command-wins. During Threshold integration on 2026-08-17, the configuration model evolved to reject duplicate targets across enabled Controllers. The original manual-command behavior remains: Web or MQTT may temporarily override an Actuator, and a later Controller transition may command it again.

## Consequences

### Positive

- local Controllers and external adapters share capability interfaces
- GPIO and concrete implementations remain below the Actuator boundary
- Actuator runtime replacement does not leave Controllers with stale concrete pointers
- persistent composition and transient control state remain explicit
- Sensor, Actuator and Controller runtimes can be rebuilt independently
- MQTT parameter state cannot trigger itself as a command

### Negative

- the three runtimes contain some parallel lifecycle mechanics
- manual external command contention with the owning Controller is currently last-command-wins
- implementation-specific Controller parameters require explicit adapter support
- retained external descriptions add a second configured-truth representation to maintain

## Alternatives considered

### One generic DeviceManager

Rejected because Sensor acquisition, physical output lifecycle and Controller coordination have different domain responsibilities despite similar composition mechanics.

### Controllers retain concrete Actuator pointers

Rejected because live Actuator rebuild would invalidate those pointers and couple Controllers to implementation lifetime.

### MQTT as an internal command/event bus

Rejected because local control must remain independent from connectivity and transport.

### Shared MQTT parameter state/command topics

Rejected because retained authoritative publications can be delivered back to EnvNode and create repeated persistence and rebuild feedback loops.
