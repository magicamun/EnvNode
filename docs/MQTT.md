# MQTT Interface

## Boundary

MQTT is an external protocol adapter. It does not form an internal event bus and domain objects do not depend on it.

Topics use:

```text
envnode/<topic-safe-device-name>/...
```

`MqttService` owns broker transport. Specialized adapters translate between MQTT and Sensor, Actuator, Controller or Value interfaces.

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

## Actuator and Controller self-description

Configured Actuator and Controller slots publish schema-1 JSON descriptions retained at:

```text
envnode/<device>/actuator/<ActuatorId>/description
envnode/<device>/controller/<ControllerId>/description
```

Descriptions are configured truth, not runtime status. They contain stable implementation,
capability, input and parameter metadata produced from slot configuration and implementation
registries. Disabled but configured slots remain described with `"enabled":false`. A `None`
slot has no description: EnvNode publishes an empty retained payload to remove any stale broker
value.

Example Actuator description:

```json
{"schema":1,"id":1,"name":"Heater","enabled":true,"implementation":"gpio_on_off","implementation_name":"GPIO On/Off","capabilities":["on_off"]}
```

Blink and Threshold descriptions additionally contain `START`/`STOP`, their target and required
capability, input metadata, parameter descriptors and current persisted parameter values. They do
not contain running state, phase, decisions, Measurement values, freshness or availability.

The publisher reconciles every fixed Actuator and Controller slot after each MQTT connection and
retries failed retained publications cooperatively. Persisted configuration changes are detected
independently of whether they came from Web or MQTT and independently of runtime application.
EnvNode does not subscribe to description topics.

Reconciliation is complete within the current MQTT device root. If the configured Device name and
therefore MQTT root changes, retained descriptions under the previous root cannot currently be
guaranteed to be removed after reboot.

## Reconnect behavior

After reconnect, command subscriptions are restored and current retained Actuator state, Controller status and Controller parameters are republished. EnvNode does not infer commands from retained state.

Controller status describes behavior and decision. Actuator status describes actual output. A manual Actuator command can therefore make actual state temporarily differ from a Controller's last phase or decision without causing MQTT-side reconciliation.

## Home Assistant

Sensor Home Assistant discovery is implemented. Controller and generic Actuator Home Assistant discovery are not implemented. Generic Actuator and Controller self-description uses the retained schema-1 topics documented above and is independent of Home Assistant discovery.

## Configurable Values

Values are defined in Web. MQTT changes only their current selection; it does not
create/delete definitions or configure Selector wiring. Display configuration
remains Web-only.

| Topic | Payload | Retained |
| --- | --- | --- |
| `envnode/<device>/value/<ValueId>/cmd/state` | Exact option code, e.g. `auto`, `cistern`, `mains` | Send commands without retain |
| `envnode/<device>/value/<ValueId>/status/state` | Current option code | Yes |
| `envnode/<device>/value/<ValueId>/description` | JSON definition and topic links | Yes |

`<device>` is the topic-safe configured device name, as for existing MQTT topics;
it is not necessarily the network hostname. IDs are stable positive Value IDs.
Commands contain 1..32 ASCII bytes (lowercase letters, digits, underscore), with no
quotes, whitespace, newline or embedded NUL. Codes are case-sensitive; display
labels are not commands. Unknown IDs/options and storage failures are rejected
and logged without changing current state. The retained status is authoritative
state, not a per-command acknowledgement.

Both Web and MQTT use ValueRuntime::set. DefaultOnRestart changes remain in RAM;
RestoreLastValue changes must be persisted successfully before changing live state.
A Selector observes the new mode in the normal controller loop. Status publishing
observes runtime independently of command source, so Web changes are also reported.
After reconnect, the adapter resubscribes and the publisher resends current states
and definitions. Subscription/publication failures are retried. A retained command
would be replayed by the broker after subscription and treated as a new command;
therefore publish commands without retain.

Example description (topic device `RainControl`, Value 1):

```json
{"schema_version":1,"id":1,"name":"Wasserquelle","value_type":"enum","default":"auto","restart_policy":"default","command_topic":"envnode/RainControl/value/1/cmd/state","state_topic":"envnode/RainControl/value/1/status/state","options":[{"code":"auto","label":"Automatik"},{"code":"cistern","label":"Zisterne"},{"code":"mains","label":"Hauswasser"}]}
```

The alternative restart policy is `restore_last`. Definitions are republished after
successful definition edits, and on reconnect. Deleted Values and changed device
names clear their previously published state/description with empty retained
messages, including retry after a failed cleanup. Tracking is bounded to eight
Values and kept in RAM: broker records from deletions/renames followed by a reboot
before cleanup may require manual removal. No Value-specific Home Assistant
discovery or configuration commands are added in this step.

Example commands (replace broker and topic device):

```sh
mosquitto_sub -h BROKER -t 'envnode/RainControl/value/+/status/state' -t 'envnode/RainControl/value/+/description' -v
mosquitto_pub -h BROKER -t 'envnode/RainControl/value/1/cmd/state' -m cistern
mosquitto_pub -h BROKER -t 'envnode/RainControl/value/1/cmd/state' -m auto
```
