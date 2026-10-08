# Domain Model

## Purpose

This document defines the functional concepts of EnvNode. Technical realization through ESP32 hardware, NVS, HTTP and MQTT is described in [TechnicalArchitecture.md](TechnicalArchitecture.md).

EnvNode follows one principle:

> Measure first. Interpret later.

Sensors acquire observations. Controllers coordinate local behavior. Actuators apply physical output. These are separate domain responsibilities.

## Core concepts

```mermaid
flowchart LR
    D[EnvNode Device] --> C[Configuration]
    D --> S[Sensors]
    S --> M[Measurements]
    D --> K[Controllers]
    K -->|requires capability| A[Actuators]
    D --> X[Diagnostics]
```

The Device is the aggregate context for one installation. It owns one authoritative Configuration and the configured Sensor, Actuator and Controller slots. Web and MQTT are external adapters, not domain objects.

## Configuration, slots and implementations

Configuration contains persistent installation choices. Runtime state is not Configuration.

A **Slot** is a fixed, stable configured identity within one domain category:

- `SensorId` identifies a Sensor slot and Measurement source.
- `ActuatorId` identifies a configured Actuator instance.
- `ControllerId` identifies a configured Controller instance.

An **Implementation** describes reusable compiled behavior, such as SHT4x, `gpio_on_off`, or Blink. A Slot selects an Implementation and supplies its instance-specific configuration. Consequently, “Actuator Slot 1” is not synonymous with `GpioOnOffActuator` or GPIO16.

## Sensor

A Sensor is one physical or simulated acquisition source. It initializes and reads its source, converts hardware-native data into canonical physical values, and emits typed Measurement content.

One Sensor may produce multiple Measurement types. SHT4x, for example, produces Temperature and RelativeHumidity. Different Sensors may produce the same type while remaining distinct sources.

Allowed Sensor processing includes conversion, calibration, compensation, filtering and debounce required to obtain a meaningful physical observation. Sensors do not perform forecasting, historical aggregation, irrigation decisions or other interpretation.

Physical and simulated Sensors use the same `ISensor` boundary and Measurement pipeline.

## Measurement

A Measurement represents an observed physical value or event. Its identity is the combination of source `SensorId` and `MeasurementType`.

A completed Measurement includes, as applicable:

- source Sensor identity
- Measurement type
- canonical value or event
- timestamp
- validity and quality
- provenance

Numeric Measurements use the canonical representation defined by their `MeasurementType`. Presentation units are external representation choices and never alter acquisition or physical meaning. Actuator state is not a Measurement merely because it can be observed externally.

The read-only [local property view](LocalProperties.md) exposes Sensor state Measurements, logical On/Off Actuator output and Threshold Controller evaluation reasons through stable references. Sensor properties reuse the existing metadata and snapshot resolver; Actuator properties read the existing output capability.

The shared `PropertyResolver` routes local references to the current Sensor, Actuator or Controller property reader. It stores no component bindings or duplicate values and resolves against the current runtime on each call. The Display page includes a six-line text preview with up to four ordered sources per line using this resolver and a bounded, typed formatter. Preview parameters remain URL inputs until explicitly saved. DisplayConfiguration stores six format strings and their ordered source references through ConfigurationService as one versioned NVS record; it has no dependency on a physical display driver.

## Actuator

An Actuator applies requested physical output. It does not acquire Measurements, decide when it should operate, or know which external adapter issued a command.

Four concepts remain distinct:

| Concept | Current example |
|---|---|
| Domain capability | `OnOff`, `Level` |
| Reusable implementation | `gpio_on_off` / `GpioOnOffActuator`, `gpio_pwm` / `GpioPwmActuator` |
| Configured instance | Actuator Slot N, identified by `ActuatorId` |
| Physical resource | `HardwareResourceAssignment` to a GPIO |

`OnOff` is represented by `IOnOffActuator`. `Level` is a validated normalized 0–100 percentage
represented by `ILevelActuator`; it is independent of PWM. Level centrally satisfies OnOff,
mapping Off to 0 and On to 100 without previous-Level restoration. A caller depends on the
capability, not on GPIO, PWM, a concrete Actuator, or a hardware assignment.

## Controller

A Controller coordinates behavior over time and controls Actuators through required capabilities. It is neither a Sensor nor an Actuator.

A configured Controller has a stable `ControllerId`, selected implementation, implementation-specific configuration and domain dependencies such as a target `ActuatorId`. Controllers do not own physical hardware assignments.

Persistent Controller configuration and runtime state are deliberately separate:

| Persistent configuration | Runtime state |
|---|---|
| enabled | running/stopped |
| implementation | current phase or decision |
| target `ActuatorId` | target available/unavailable |
| implementation parameters | last operation result |

START and STOP are transient runtime operations. They do not rewrite persistent `enabled`. An enabled Controller may therefore be manually stopped and started again without a configuration rebuild. Blink START begins a fresh Blink cycle. Threshold START resets decision to Unknown and immediately evaluates a currently usable snapshot when one exists. STOP halts behavior and requests target Off where possible.

## BlinkController

Blink is the first Controller implementation and an architectural proof, not the final purpose of the Controller model. Its typed configuration contains target `ActuatorId`, On duration and Off duration. Blink requires the target implementation to advertise `OnOff`.

It uses a monotonic clock and cooperative, non-blocking servicing:

```text
START -> resolve target -> On -> wait On duration
      -> Off -> wait Off duration -> repeat

STOP  -> stop transitions -> request target Off where available
```

Blink does not depend on GPIO, `GpioOnOffActuator`, MQTT, Web, wall-clock time or NTP.

## MeasurementSourceReference and ThresholdController

A Controller input identifies a specific Measurement stream with `MeasurementSourceReference`:

- `SensorId`
- `MeasurementType`

This is more precise than identifying only a Sensor because one Sensor implementation may advertise several Measurement types. Sensor names, implementations, GPIO assignments, MQTT topics and runtime Sensor pointers are not part of Measurement identity.

Threshold is the implemented Measurement-driven Controller. Its typed configuration contains one `MeasurementSourceReference`, target `ActuatorId`, `onThreshold`, `offThreshold` and `maxMeasurementAgeMs`. It accepts only Measurement metadata classified as `FloatingPoint` and `State`; Boolean state, event and numeric event/delta Measurements are excluded.

Its hysteresis decision is:

```text
value >= onThreshold               -> On
value <= offThreshold              -> Off
offThreshold < value < onThreshold -> retain previous decision
```

The initial decision is `Unknown`. A first usable in-band Measurement leaves it Unknown and issues no Actuator command. Missing, invalid or stale input creates no new decision and does not force Off. The previous decision remains diagnostically visible. Explicit STOP is different: it stops evaluation, resets the decision lifecycle and requests target Off where possible.

Threshold freshness uses monotonic elapsed time from the snapshot's `acceptedMonotonicMs`, not NTP, timezone, wall-clock Measurement timestamp or Sensor schedule. `MeasurementQuality` remains visible but Good, Estimated and Degraded are all accepted when the Measurement is otherwise valid, compatible and fresh; no generic Controller quality policy exists.

`MeasurementSnapshotCache` stores bounded latest snapshots indexed by `SensorId + MeasurementType`. Each snapshot includes monotonic acceptance time and a revision identifying accepted changes. `IMeasurementResolver` returns copied snapshots through a narrow read-only interface, so Controllers do not depend on cache internals or retain Sensor pointers. It is neither a history buffer nor an event bus.

A SensorRuntime rebuild clears SensorManager and the snapshot cache. Controllers then cannot evaluate the old composition's snapshots; a replacement Sensor must emit a new Measurement. ControllerRuntime need not be rebuilt merely because the implementation behind the same `SensorId` is replaced while the referenced Measurement remains compatible.

## Capability resolution and runtime replacement

Controllers retain `ActuatorId`, not a long-lived pointer to a concrete Actuator. Each operation resolves the current capability through:

```text
IOnOffActuatorResolver
    -> ActuatorRuntime
    -> current IOnOffActuator for ActuatorId
```

If Actuator Slot 1 is rebuilt from GPIO16 to GPIO17, a Controller still targets ActuatorId 1 and resolves the replacement runtime instance. This is deliberate domain decoupling and stale-pointer/lifetime protection.

Configuration-time compatibility and runtime availability are different. Validation ensures an enabled Controller references a valid enabled Actuator implementation advertising its required capability. Initialization may still fail or the runtime capability may be temporarily unavailable.

Measurement-driven Controllers likewise retain `SensorId + MeasurementType`, not Sensor pointers, and resolve copied snapshots through `IMeasurementResolver`. SensorRuntime, ActuatorRuntime and ControllerRuntime can therefore be rebuilt independently when stable identities and capabilities remain compatible.

## Configuration integrity

ConfigurationService validates the complete candidate composition before persistence. An enabled Threshold Controller requires an enabled source Sensor whose selected implementation advertises the referenced Threshold-compatible Measurement, plus an enabled target Actuator advertising `OnOff`. Enabled Blink requires an enabled `OnOff` target. Reverse integrity rejects changing a referenced Sensor to an incompatible source or disabling/removing/making a referenced Actuator capability-incompatible. Duplicate targets across enabled Controllers are rejected regardless of Controller implementation. Web and MQTT filtering never replace these authoritative checks.

## Hardware ownership

Configured Sensors and Actuators may own `HardwareResourceAssignment`s. Controllers never do.

Board capability validation answers whether a resource can support an implementation. Unified occupancy validation answers whether enabled Sensor and Actuator slots may use their assignments together:

- identical exclusive GPIO assignments conflict
- Sensor/Actuator and Actuator/Actuator GPIO collisions conflict
- identical I2C bus/address claims conflict where applicable
- different addresses may share one I2C bus
- disabled slots do not actively claim hardware

Web filtering is only a convenience. Configuration validation remains authoritative.

## External adapters and contention

Web and MQTT translate external requests into the same configuration, runtime and capability interfaces. They do not manipulate GPIO or concrete Controller/Actuator implementations.

Only one configured enabled Controller may target a given Actuator. A disabled Controller does not claim its configured target, but transient runtime STOP does not release ownership: the Controller remains configured enabled. This prevents Controller-versus-Controller contention without runtime locks or arbitration.

Manual Web and MQTT Actuator commands remain allowed while a Controller owns the target. Current behavior is last-command-wins. Controllers do not continuously reconcile actual Actuator state with an internal phase or decision. For example, after Threshold commands On, a user may command Off; Threshold does not immediately reassert On every loop, but a later decision transition may command again. No priority, lease or manual-versus-Controller arbitration model exists.

## Future multi-input Controllers

Future typed implementations may contain multiple `MeasurementSourceReference` values when a real behavior requires them. A future RainDetectorController might combine rain/wet or detector-level input with outside and detector-surface temperatures. EnvNode intentionally does not introduce Boolean-expression syntax, scripting or a generic rule engine. Generalization should follow multiple demonstrated use cases.

## Current status

### Implemented and verified

Controller infrastructure v1 is implemented, tested and physically verified for the current Blink and Threshold/Hysteresis feature set. This establishes the runtime, configuration and adapter boundaries without claiming that all future Controller needs are solved.

- fixed Sensor slots, implementation registry, factory/runtime composition and scheduling
- typed Measurement pipeline, snapshots and MQTT publication
- fixed Actuator slots and stable `ActuatorId`
- Actuator implementation registry and capability metadata
- `OnOff`, `IOnOffActuator` and `GpioOnOffActuator`
- unified Sensor/Actuator hardware occupancy validation
- deterministic Actuator construction, safe shutdown and live rebuild
- Web and MQTT Actuator control with command-source-independent state publication
- fixed Controller slots and stable `ControllerId`
- Controller implementation registry, factory and runtime
- BlinkController with cooperative non-blocking service
- ThresholdController with MeasurementSourceReference, hysteresis and monotonic freshness
- bounded MeasurementSnapshotCache and read-only IMeasurementResolver
- live Controller rebuild and transient runtime START/STOP
- exclusive enabled-Controller target ownership
- Web Controller configuration, compatible source/target filtering, diagnostics and control
- MQTT Controller status, START/STOP, retained Blink/Threshold parameter state and persistent parameter commands
- capability resolution across Actuator runtime replacement
- physically verified Sensor -> Measurement -> Controller -> Actuator behavior

### Deliberately future or not implemented

- additional Actuator capabilities beyond OnOff and Level
- additional Controller implementations
- multi-input, Boolean/contact and event-driven Controller semantics
- RainDetector-specific control behavior
- atomic external mutation of structural Measurement source configuration
- generic Controller parameter-description schema
- Controller Home Assistant discovery
- generic Actuator Home Assistant discovery
- richer manual-versus-Controller arbitration
- scripting/rule engine and generic command/event bus

### Optional text display output

DisplayConfiguration owns a six-line page plus optional SSD1309 I2C output
settings. ConfigurationService validates and stores it as one versioned NVS record.
DisplayService formats snapshots through IPropertyReader and writes complete pages
through ITextDisplay. Ssd1309TextDisplay implements that boundary using the existing
I2CBusManager; Application schedules the display service after normal runtime work.
DisplayPageFormatter is shared with the web preview. Display output does not
publish MQTT state or change sensor, actor or controller decisions.

Boolean display labels are optional presentation overrides per ordered source
position, stored with DisplayConfiguration. PropertyTextFormatter applies them
only when the actual typed source is Boolean. Empty labels use PropertyDescription
fallbacks; enum and numeric values retain their existing rendering. Overrides do
not mutate the property reader or affect MQTT, actuators or controller decisions.

Text properties own their string payload. TimePropertyReader provides locale-aware
System properties (component 1: date/time/datetime) through an optional system
reader in PropertyResolver. ActuatorPropertyReader optionally binds ILevelActuator
for an unsigned percentage property. Display-only enum overrides match stable
PropertyEnumOption codes; empty/missing overrides use the original label.
ConfigurationService migrates older display strings into a bounded version-4 blob
when saved, preserving the single-record write boundary.

### Configurable Values: first model step

A Value is named, typed local input supplied by an operator or external system.
It is neither a sensor measurement nor an actuator. The first implementation is
EnumValue: an owned definition with a nonzero ID, name, up to 16 options (stable
code and label), a required default code and explicit restart policy. Codes are
case-sensitive lowercase ASCII letters, digits and underscores, at most 32 bytes.
Names and labels are 1..64 UTF-8 bytes without ASCII control characters.

`DefaultOnRestart` starts from the configured default. `RestoreLastValue` accepts
a previously saved code at begin; absent or obsolete saved codes fall back to the
default with an explicit result. Unknown commands never change the current value.
Only successful value changes increment the session revision. Calling begin
starts a new session and resets revision to zero. Definitions are copied and
validated before use; invalid definitions cannot accept writes.

The model and NVS storage are implemented. Runtime composition and Web commands are implemented. MQTT commands and retained state/description publication are implemented for Values. The restart policy specifies behavior; EnumValue does not
perform storage I/O. ValueRuntime saves successfully before acknowledging a
persistent change. No installation-specific mode is
created automatically and no existing actuator ownership changes.

Example definition (application configuration, not a built-in water mode):

```cpp
EnumValueConfiguration config;
config.id = 1;
config.name = "Wasserquelle";
config.options = {{"auto", "Automatik"}, {"cistern", "Zisterne"}, {"mains", "Hauswasser"}};
config.defaultCode = "auto";
config.restartPolicy = ValueRestartPolicy::DefaultOnRestart;
EnumValue mode(config);
mode.begin();
mode.set("cistern"); // Changed; a future Web/MQTT adapter uses this same operation.
```

### Value storage

ConfigurationService now stores up to eight EnumValue definitions under the
versioned NVS blob `enum_values1` in the existing weather namespace. IDs must be
unique and remain independent of ordering. Definition changes preserve a saved
code only when the same ID still allows that code and uses RestoreLastValue.
Switching to DefaultOnRestart, deleting a Value or removing its saved option
clears that checkpoint; a later restart then uses the configured default.

Definitions and restart checkpoints share one validated record. Failed writes
leave the in-memory configuration untouched. Unknown IDs/codes and checkpoints
for DefaultOnRestart are rejected. Re-saving the same checkpoint avoids a write.
Missing or invalid records load as an empty collection; existing device settings
remain intact. Factory reset removes Values together with other configuration.
The storage API is not a live command API: ValueRuntime provides the shared live command API used by Web and MQTT. No Value is created automatically on upgrade.

### Values Web page and runtime

`/values` lists current values and provides immediate POST commands. `/values/edit`
creates or edits a definition; an optional water-source example pre-fills an
unsaved form. No installation-specific Value is created automatically. The form
supports names, up to 16 key/label pairs, a default key and restart policy. IDs are
allocated from unused positive IDs and remain fixed when editing. Removing a
Value is an explicit POST operation in its editor.

ValueRuntime is composed independently of Web and initialized after configuration
load. Transient commands update RAM only. RestoreLastValue commands persist before
updating live state; failed writes preserve the old state. Definition edits apply
immediately and restart only the edited Value from its compatible saved code or
default; unrelated Values retain their live state. Invalid submissions retain the
entered definition for correction. Values supply Selector mode inputs and accept Web/MQTT commands; they do not directly command actuators. Their enum state is
available as `value/ID/state` to both display preview and physical display.

Manual acceptance: open Values, choose Water source example, save, select Zisterne
and Set value. Reload to confirm the current value. With Use default, a reboot
returns to Automatik. Edit to Restore last value, save, select Hauswasser, then
reboot: Hauswasser should remain. Deleting the Value should survive reboot too.

### Value display properties

ValuePropertyReader resolves each read against ValueRuntime and exposes the enum
state as `value/ID/state`. `%s` uses the configured option label; optional display
translations override labels by stable code. The Display source inventory includes
all live Values. Saving a display page preserves their enum translations. Switching
a Value is picked up by the next normal OLED refresh; the Web preview updates on
request. Deleted Values yield UnknownReference without keeping runtime pointers.
Dynamic enum snapshots and descriptions own their metadata, including across
copies, definition edits and deletions. Static enum properties retain their
existing borrowed static metadata.

### Selector controller

A decision-only Threshold evaluates hysteresis without owning or writing an
actuator, including during start/stop. Existing Threshold configurations retain
direct output by default. A Selector maps an Enum Value's three configured codes
to automatic Threshold decision, constant On and constant Off. Its automatic
source must be an enabled decision-only Threshold, preventing cycles and double
output ownership. Source Thresholds run before Selectors regardless of slot order.

Unknown/stale/invalid/stopped automatic input or an unavailable/unmapped mode
causes no actuator write and cancels pending output. On startup this preserves the
actuator's initialized state. Manual On/Off ignores automatic input validity.
Selector stop also holds the output. Valid decisions are reconciled against the
logical actuator state; failed commands retry only while that decision is valid.
The Selector alone owns its target. Mapping edits use the existing explicit
Controller Save/Apply flow; Value commands take effect on the next runtime loop.

Selector setup in Web:

1. Edit the existing Threshold, enable **Decision only (no actuator control)**,
   then Save Slot. Its numeric thresholds and direction still determine On/Off.
2. Configure another enabled Controller Slot as **Selector**. Select the mode
   Value and the decision-only Threshold. Map the automatic option, the option
   that forces On, and the option that forces Off; choose the valve actuator.
3. Save Slot and apply the Controller composition. No automatic migration changes
   an existing installation's behavior before an explicit configuration/apply.
4. Switch the Value on the Values page. Auto uses the current Threshold decision;
   On/Off modes override it. Stop the Threshold while in Auto to verify hold,
   then verify manual modes still work. Restart the Threshold to resume Auto.

Existing direct Thresholds retain their established stop/rebuild behavior. Once
configured as decision-only they never write an output. Selectors hold on stop,
missing mode, unmapped mode, or unknown automatic decision. Actuator readback is
logical command state, not mechanical valve feedback. Used mode options and source
Thresholds cannot be removed/disabled in saved configuration while an enabled
Selector references them. Runtime STOP remains available for testing/operation.

Selector status is included in existing Controller MQTT status publication.
Chains of Selectors are not supported; Value MQTT commands use the shared runtime.
Controller runtime
entries and staging are bounded, checked heap allocations, reducing ESP32 static
DRAM and apply-call stack pressure. A staging allocation failure leaves the active
composition intact. Runtime diagnostics use a compact mapping signature solely
for the pending-apply indicator, never for control decisions.

### Value MQTT integration

ValueMqttAdapter validates transport input and delegates to ValueRuntime; it owns
subscription retry/reconnect behavior. ValueStatePublisher independently observes
live Values and publishes retained codes and enum descriptions. Definition revision
changes trigger metadata updates without rebuilding JSON on every loop. The MQTT
router adds a third optional handler without replacing Actuator/Controller routes.
Application schedules the Value adapters after MQTT processing and before the
Controller loop. There is no MQTT-to-Display dependency. See MQTT.md for topics,
retention semantics and cleanup limitations.
