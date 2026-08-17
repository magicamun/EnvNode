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
    RT --> I[IOnOffActuator]
    I --> G[GpioOnOffActuator]
    G --> P[GPIO output]
```

`ActuatorId` is the stable identity of a configured Slot. The registry describes reusable implementations and their capability and hardware requirements. `slot.hardware` is the single source of truth for the instance's `HardwareResourceAssignment`.

The current implementation is `gpio_on_off`. It requires a GPIO with `DigitalOutput` capability and exposes the `OnOff` domain capability through `IOnOffActuator`.

An adapter or Controller addresses an Actuator Slot and capability. It never addresses `GpioOnOffActuator` or GPIO directly.

## Capabilities

`OnOff` is currently the only implemented Actuator capability:

- `Off`
- `On`

Capability metadata lets clients request behavior without depending on its physical implementation. A future relay, remote output or other implementation may expose the same `OnOff` interface.

Level/percentage control remains future architecture. It is not currently implemented.

## Runtime ownership and rebuild

`ActuatorRuntime` owns the application-lifetime runtime composition and capability lookup by `ActuatorId`. `ActuatorFactory` uses fixed, aligned storage for deterministic construction.

A live rebuild stages a replacement composition, safely shuts down old Actuators, and activates the new composition without restarting the ESP32. Shutdown requests `Off` before releasing GPIO ownership where the implementation can do so.

Actuator state is independent of command source. Web, MQTT and Controllers all reach the same `IOnOffActuator`; `ActuatorStatePublisher` observes actual state and publishes it independently.

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

The first Controller implementation is Blink:

```mermaid
flowchart LR
    C[ControllerSlotConfiguration] --> CR[ControllerRuntime]
    CR --> B[BlinkController]
    B -->|ActuatorId| R[IOnOffActuatorResolver]
    R --> A[ActuatorRuntime]
    A --> I[IOnOffActuator]
```

Blink retains an `ActuatorId`, not a concrete pointer. It resolves the current `IOnOffActuator` for each operation. An Actuator runtime rebuild may therefore replace or move the physical implementation without leaving the Controller with a stale pointer.

Blink contains timing and sequencing; the basic Actuator does not. Its cooperative monotonic state machine commands On and Off only at scheduled transitions. STOP cancels future transitions and requests Off.

## Contention

Current command behavior is deliberately simple: last command wins. A manual Web or MQTT Actuator command can temporarily change state while Blink is running; Blink may overwrite it at its next scheduled transition. No ownership, priority or arbitration model is implemented.
