# MQTT Interface

## Boundary

MQTT is an external protocol adapter. It does not form an internal event bus and domain objects do not depend on it.

Topics use:

```text
envnode/<topic-safe-device-name>/...
```

`MqttService` owns broker transport. Specialized adapters translate between MQTT and Sensor, Actuator or Controller interfaces.

## Sensor Measurements

```text
envnode/<device>/sensor/<SensorId>/<measurement-type>
```

Measurements identify their stable source and type. They are published non-retained through `MeasurementPublisher`. Sensors never publish MQTT directly.

## Actuator runtime control and state

The implemented On/Off topics are:

```text
envnode/<device>/actuator/<ActuatorId>/cmd/on_off
envnode/<device>/actuator/<ActuatorId>/status/on_off
```

Commands accept `ON` and `OFF`. Status is retained and reflects actual `IOnOffActuator` state independently of whether Web, MQTT or a Controller produced the change.

## Controller runtime status

```text
envnode/<device>/controller/<ControllerId>/status
```

Status is retained runtime truth. Blink currently publishes a compact JSON object such as:

```json
{"running":true,"phase":"on","target_available":true,"last_result":"completed"}
```

It changes after Start/Stop, Blink phase transitions, target availability or operation-result changes, runtime rebuild and MQTT reconnect. It does not duplicate target Actuator state.

Threshold publishes this exact field contract:

```json
{"running":true,"source_available":true,"measurement_valid":true,"stale":false,"decision":"on","target_available":true,"output_pending":false,"last_result":"completed"}
```

`decision` is `unknown`, `on` or `off`; Unknown is not mapped to Off. Status represents current Controller diagnostics and does not reinterpret missing, invalid or stale input as an Off command. It intentionally omits the latest numeric value because Sensor MQTT remains authoritative for Measurement telemetry. Change detection prevents identical status from being published every application loop.

## Controller runtime commands

```text
envnode/<device>/controller/<ControllerId>/cmd
```

Accepted payloads are exact `START` and `STOP` values. They call `ControllerRuntime` operations and are never persisted. STOP does not set configured `enabled=false`; START does not set `enabled=true`.

## Controller parameter state and commands

Implementation-specific parameter state is published retained and is authoritative persisted configuration. Blink uses:

```text
envnode/<device>/controller/<ControllerId>/parameter/on_duration_ms
envnode/<device>/controller/<ControllerId>/parameter/off_duration_ms
```

These are authoritative persisted values. EnvNode does not subscribe to these topics.

Threshold uses:

```text
envnode/<device>/controller/<ControllerId>/parameter/on_threshold
envnode/<device>/controller/<ControllerId>/parameter/off_threshold
envnode/<device>/controller/<ControllerId>/parameter/max_measurement_age_ms
```

External mutation requests use separate, non-retained command topics:

```text
envnode/<device>/controller/<ControllerId>/cmd/parameter/on_duration_ms
envnode/<device>/controller/<ControllerId>/cmd/parameter/off_duration_ms
```

Threshold mutation topics are:

```text
envnode/<device>/controller/<ControllerId>/cmd/parameter/on_threshold
envnode/<device>/controller/<ControllerId>/cmd/parameter/off_threshold
envnode/<device>/controller/<ControllerId>/cmd/parameter/max_measurement_age_ms
```

Blink durations and Threshold maximum age use decimal milliseconds. Threshold values use finite, locale-independent decimal floating-point syntax. Complete candidate validation enforces threshold ordering, freshness range, source compatibility, target capability and exclusive enabled-Controller target ownership.

The mutation path is:

```text
MQTT cmd/parameter
 -> ControllerMqttAdapter
 -> candidate ControllerSlotConfiguration
 -> IConfigurationService
 -> validation and persistence
 -> RestartControllerRuntime
 -> ControllerRuntime rebuild
 -> retained parameter state
```

An unchanged value is accepted as a no-op: it is not persisted and does not rebuild the runtime. Invalid or rejected requested values are never published as authoritative state.

Separating `parameter/...` state from `cmd/parameter/...` mutation prevents EnvNode from consuming its own retained publication and creating a configuration feedback loop.

Web and MQTT share the same persistent Controller configuration. Web changes therefore update retained MQTT parameter state, and accepted MQTT changes appear in Web administration.

Threshold's `MeasurementSourceReference` is structural (`SensorId + MeasurementType`) and is intentionally not exposed as independently mutable scalar MQTT parameters. There are no `source_sensor_id` or `source_measurement_type` mutation topics; source selection currently uses the complete configuration/Web path to avoid invalid intermediate references.

## Reconnect behavior

After reconnect, command subscriptions are restored and current retained Actuator state, Controller status and Controller parameters are republished. EnvNode does not infer commands from retained state.

Controller status describes behavior and decision. Actuator status describes actual output. A manual Actuator command can therefore make actual state temporarily differ from a Controller's last phase or decision without causing MQTT-side reconciliation.

## Home Assistant and self-description

Sensor Home Assistant discovery is implemented. Controller discovery, generic Actuator discovery, and generic external Actuator/Controller capability or parameter self-description are not implemented.
