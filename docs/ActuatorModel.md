# Actuator Model

## Purpose

EnvNode distinguishes between sensing, decision-making, and physical output.

- **Sensors** observe the environment and produce `Measurement`s.
- **Controllers** consume `Measurement`s and decide what should happen.
- **Actuators** apply a requested physical output.
- **Adapters**, such as MQTT, expose or control these concepts externally but are not part of the internal control model.

An actuator must not contain application-specific control logic.

Its responsibility is limited to applying a requested output state to hardware.

## Design Principle

Actuators describe **what can be controlled**, not **how the hardware implements it**.

Examples:

- GPIO output
- relay
- MOSFET
- PWM output
- DAC output

These are implementation details.

A controller should not need to know whether an actuator drives an LED, a relay, or the heater MOSFET of a RainDetector.

## Initial Capabilities

EnvNode initially supports two actuator capabilities.

### OnOff

An `OnOff` actuator supports two logical states:

- `Off`
- `On`

Typical examples:

- relay
- valve
- digital output
- LED
- heater enable

The physical implementation may use a GPIO or another output mechanism.

### Level

A `Level` actuator accepts a normalized value from:

`0 ... 100 %`

Semantics:

- `0 %` means **Off**
- `100 %` means maximum output
- values between `0 %` and `100 %` represent proportional output

Typical examples:

- PWM-controlled heater
- dimmable LED
- fan speed
- proportional valve

The percentage is a domain-level value.

A caller must not need to know the PWM resolution, PWM frequency, DAC range, GPIO implementation, or other hardware details.

## Capability Relationship

A `Level` actuator can functionally represent an Off state by setting its level to `0 %`.

However, `OnOff` and `Level` remain distinct capabilities.

A pure `OnOff` actuator does not support intermediate values.

A `Level` actuator does.

This distinction allows Controllers to declare the capability they require.

## No Timing Logic Inside Basic Actuators

Basic actuators do not implement timing behavior such as:

- blinking
- periodic switching
- delays
- pulse sequences
- schedules
- regulation loops

For example, a blinking LED is still an `OnOff` actuator.

The sequence

`On -> Off -> On -> Off`

is produced by a Controller or another higher-level control component.

This keeps the actuator deterministic and reusable.

## Hardware Independence

The first implementation may use an LED connected through a resistor to an ESP32 GPIO.

This is only a test implementation.

The same actuator abstraction must later be usable for hardware such as:

`ESP32 GPIO -> MOSFET -> RainDetector heater`

without requiring application logic to change.

## Hardware Resource Integration

Actuators use the existing EnvNode hardware resource model.

An actuator declares the capabilities required from its assigned hardware resource.

Hardware resource assignments are validated through `BoardCapabilities`.

Actuators must not introduce a separate GPIO or hardware resource configuration model.

The hardware resource model distinguishes between:

- the capabilities provided by the board,
- the hardware resource assigned to an actuator,
- and whether that resource is already in use.

For example, an `OnOff` actuator using a GPIO requires
`GpioCapability::DigitalOutput`.

The actuator must not depend directly on an arbitrary GPIO number without
going through the existing hardware resource assignment and validation model.

## Initial Development Steps

The first actuator implementation should be deliberately small.

1. Introduce the actuator abstraction.
2. Implement an `OnOff` GPIO actuator.
3. Use an LED with a resistor as test hardware.
4. Verify explicit `On` and `Off` commands.
5. Add external timing logic that periodically switches the actuator to demonstrate blinking.
6. Only after the `OnOff` model is stable, introduce the `Level` capability and PWM-based output.

MQTT integration and measurement-based Controllers are deliberately outside this first implementation step.

## Architectural Constraints

- Actuators must not depend on MQTT.
- Actuators must not consume Sensor Measurements directly.
- Actuators must not make control decisions.
- Hardware-specific details must remain behind the actuator implementation.
- Controllers depend on actuator capabilities, not on concrete GPIO or PWM implementations.
- `Level` values exposed to the domain are normalized to `0 ... 100 %`.
- `Level(0)` is defined as Off.

## Guiding Example

For the initial LED experiment:

`Controller / test logic`
↓
`OnOff capability`
↓
`GPIO actuator`
↓
`ESP32 GPIO`
↓
`LED + resistor`

Later:

`Controller`
↓
`Level capability`
↓
`PWM actuator`
↓
`ESP32 PWM`
↓
`MOSFET`
↓
`RainDetector heater`

The upper layers should not depend on the physical load connected to the output.