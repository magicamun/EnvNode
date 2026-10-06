# Actuator and Controller Model

## Responsibility split

- Sensors observe physical reality and produce typed `Measurement`s.
- Controllers coordinate behavior over time.
- Actuators apply requested physical output through typed capabilities.
- Web and MQTT adapt external requests to the same internal interfaces.

Actuator state is runtime control state, not a Measurement. Controllers are not Sensors or Actuators.

## Configured Actuators

The implemented composition path is:

```mermaid
flowchart LR
    C[ActuatorSlotConfiguration] --> R[ActuatorImplementationRegistry]
    C --> F[ActuatorFactory]
    F --> RT[ActuatorRuntime]
    RT --> O[IOnOffActuator]
    RT --> L[ILevelActuator]
    O --> G[GpioOnOffActuator]
    L --> P[GpioPwmActuator]
    G --> H[GPIO output]
    P --> H
```

`ActuatorId` is the stable identity of a configured Slot. The registry describes reusable implementations and their capability and hardware requirements. `slot.hardware` is the single source of truth for the instance's `HardwareResourceAssignment`.

`gpio_on_off` exposes the `OnOff` domain capability through `IOnOffActuator`.
`gpio_pwm` exposes normalized `Level` through `ILevelActuator` and implements it with ESP32
LEDC PWM. Both require a GPIO with `DigitalOutput`; PWM frequency, resolution, channel and
duty remain implementation details.

An adapter or Controller addresses an Actuator Slot and capability. It never addresses `GpioOnOffActuator` or GPIO directly.

## Capabilities

The implemented Actuator capabilities are:

- `OnOff`: `Off` and `On`
- `Level`: normalized integer percentage 0–100

`Level` centrally satisfies `OnOff`: Off maps to 0%, On maps to 100%, and On never restores
a previous partial Level. An OnOff-only implementation cannot satisfy a Level requirement.
Capability metadata lets clients request behavior without depending on its physical implementation.

The GPIO PWM implementation starts and shuts down at Level 0. Runtime Level is not persisted.

## Runtime ownership and rebuild

`ActuatorRuntime` owns the application-lifetime runtime composition and capability lookup by `ActuatorId`. `ActuatorFactory` uses fixed, aligned storage for deterministic construction.

A live rebuild stages a replacement composition, safely shuts down old Actuators, and activates the new composition without restarting the ESP32. Shutdown requests `Off` before releasing GPIO ownership where the implementation can do so.

Actuator state is independent of command source. Web, MQTT and Controllers resolve typed
capabilities through `ActuatorRuntime`; `ActuatorStatePublisher` observes OnOff and Level state
independently. MQTT Level commands use `actuator/<slot>/cmd/level`, with retained authoritative
state on `actuator/<slot>/status/level`.

## Hardware validation

Actuators share EnvNode's hardware capability and occupancy infrastructure with Sensors:

- `BoardCapabilities` validates whether an assigned resource provides required capabilities.
- unified occupancy validation detects conflicts across enabled Sensor and Actuator slots
- exclusive GPIO reuse conflicts
- identical I2C bus/address claims conflict where applicable
- different addresses may share an I2C bus
- disabled slots do not claim hardware

Controllers do not own hardware assignments and do not participate in occupancy validation.

## Controllers and capability resolution

The implemented Controller composition and Measurement-driven path are:

```mermaid
flowchart LR
    S[Sensor] --> M[Measurement]
    M --> C[MeasurementSnapshotCache]
    C --> MR[IMeasurementResolver]
    CC[ControllerSlotConfiguration] --> REG[ControllerImplementationRegistry]
    REG --> F[ControllerFactory]
    F --> CR[ControllerRuntime]
    CR --> B[BlinkController]
    CR --> T[ThresholdController]
    MR --> T
    B -->|ActuatorId| R[IOnOffActuatorResolver]
    T -->|ActuatorId| R
    R --> A[ActuatorRuntime]
    A --> I[IOnOffActuator]
```

Both Controllers retain an `ActuatorId`, not a concrete pointer. They resolve the current `IOnOffActuator` when needed. An Actuator runtime rebuild may therefore replace or move the physical implementation without leaving a stale pointer.

Blink contains timing and sequencing; the basic Actuator does not. Its cooperative monotonic state machine commands On and Off only at scheduled transitions. STOP cancels future transitions and requests Off.

Threshold consumes one copied Measurement snapshot identified by `MeasurementSourceReference` (`SensorId + MeasurementType`). It accepts `FloatingPoint` + `State` metadata, evaluates `onThreshold`/`offThreshold` hysteresis and uses `maxMeasurementAgeMs` against monotonic acceptance time. It retains neither a Sensor pointer nor cache storage. Missing, invalid or stale input produces no new decision and does not automatically force Off.

`ControllerRuntime` owns Controller instances and supports staged live replacement. Controller configuration changes request only `RestartControllerRuntime`; they do not inherently restart SensorRuntime or ActuatorRuntime.

## Controller target ownership

One configured enabled Controller may target an Actuator. A second enabled Blink or Threshold targeting the same `ActuatorId` is rejected across implementation types. Disabled Controllers do not claim a target, but runtime STOP does not release ownership because persistent `enabled` remains true. ConfigurationService is authoritative; Web filtering is convenience only.

## Contention

Controller-versus-Controller contention is prevented by exclusive enabled-Controller target ownership. Manual Web or MQTT Actuator commands remain deliberately last-command-wins relative to the owner. A manual change may remain until Blink's next scheduled transition or Threshold's next decision transition; Controllers do not continuously reconcile actual output state. No priority, lease or manual-versus-Controller arbitration model is implemented.

## Descriptor-backed actuator configuration

A saved actuator slot may bind to a module instance and descriptor device using
`ModuleActuatorReference`, shared with controller targets. In that case only the
user name and enabled state are configurable. Implementation and GPIO are resolved
from the descriptor at runtime; saved module slots contain no GPIO assignment.
The binding survives reboot and moving the module to another connector. An absent
or replaced module leaves the saved slot unavailable, without falling back to GPIO.
Unconfigured descriptor actuators remain automatically enabled. Explicitly disabled
module devices remain reserved and are not recreated in another runtime slot.
The web editor distinguishes descriptor On/Off devices from manual GPIO/PWM slots.
I2C diagnostics annotate EEPROM addresses using the existing discovery inventory;
a bus scan itself does not identify devices or refresh descriptor discovery.
The mainboard identity EEPROM at I2C0 / 0x50 is labeled separately, with its
resolved board identity status and source; EEPROM-derived name and revision are
shown only when the identity source is EEPROM.

The web module inventory uses a temporary heap buffer while rendering the device
page, avoiding a permanent static DRAM reservation and a large task-stack object.
Allocation failure is reported in the inventory table.

I2C bus scans run once during boot after bus initialization and module discovery.
I2CBusManager keeps the latest result per bus in RAM. Diagnostics reads this
snapshot without probing again; the manual scan replaces it for all board buses.
Scan failures remain diagnostic and do not stop startup.
