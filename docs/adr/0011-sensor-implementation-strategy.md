# ADR-0011

# Sensor Implementation Strategy and Hardware Resource Assignment

- Status: Accepted
- Date: 2026-08-08

---

## Context

WeatherStation now supports multiple Sensor implementations.

Current examples include:

- Simulated Temperature
- Simulated Humidity
- Simulated Pressure
- AM2302 / DHT22

Future implementations include:

- SHT4x
- BMP390
- ADS1115-based sensors
- AS5600
- Rain Gauge
- DS18B20
- other GPIO-, I2C-, OneWire-, ADC- or SPI-based Sensors

The current runtime composition is still largely defined statically in the composition root.

As the number of supported Sensors grows, the Device must be able to configure:

- which Sensor implementations are used
- which logical Sensor Slot they occupy
- whether they are enabled
- which hardware resources they require
- implementation-specific parameters
- acquisition scheduling

This must not require firmware source changes for every installation.

At the same time, hardware resources are not interchangeable.

Examples:

- GPIO pins may be exclusive
- I2C devices share a bus but must not collide by address
- OneWire devices share a bus and may require ROM identity
- ADC channels may be individually assigned
- some ESP32 GPIOs are reserved or unsuitable
- board-specific connectors may map to fixed MCU resources

A free-form configuration model would make invalid or conflicting assignments easy to create.

WeatherStation therefore requires an explicit Sensor implementation registry and hardware resource assignment model.

---

## Decision

WeatherStation separates three concepts:

1. Sensor Implementation
2. Sensor Slot Configuration
3. Hardware Resource Assignment

Conceptually:

    Sensor Implementation Registry
                |
                v
        Sensor Slot Configuration
                |
                v
          Sensor Factory
                |
                v
           Runtime Sensor
                |
                v
          SensorManager

The registry defines what the firmware is capable of.

The Slot configuration defines what this Device should instantiate.

The hardware assignment defines which physical resources the selected implementation may use.

## Implementation status (2026-08-17)

Implemented for Sensors and extended, without changing Sensor domain ownership, to Actuator Slots. Board capability validation is separate from one unified occupancy validation across enabled Sensor and Actuator assignments. Exclusive GPIO reuse is rejected across both categories; I2C devices may share a bus when addresses differ. Controllers own no hardware assignment and do not participate in occupancy validation.

---

## Sensor Implementation Registry

The firmware contains one authoritative registry of compiled Sensor implementations.

Conceptually:

    SensorImplementation

        SimulatedTemperature
        SimulatedHumidity
        SimulatedPressure
        AM2302
        SHT4x
        BMP390
        ...

Each implementation has stable typed identity.

Free-form class names or arbitrary strings are not used as implementation identity.

The registry provides implementation metadata.

---

## Implementation Metadata

Each Sensor implementation describes at least:

- stable implementation identifier
- human-readable type name
- provenance class
- supported MeasurementTypes
- hardware interface kind
- default acquisition mode
- default sample interval
- required implementation-specific configuration

Example:

    AM2302

    Type:
        AM2302 / DHT22

    Provenance:
        Physical

    Measurements:
        Temperature
        RelativeHumidity

    Interface:
        Digital GPIO / Custom single-wire protocol

    Default schedule:
        Periodic, 5000 ms

    Configuration:
        GPIO

Implementation metadata describes capabilities.

It does not describe one configured Sensor instance.

---

## Sensor Slot Configuration

A Sensor Slot describes one logical Sensor source within the Device.

Conceptually:

    SensorSlotConfiguration
        |
        +---- SensorId / SlotId
        +---- enabled
        +---- name
        +---- implementation
        +---- schedule
        +---- hardware assignment
        +---- implementation-specific configuration

The Slot ID remains the stable runtime SensorId defined by ADR-0010.

The user-visible name is independent from the implementation.

Changing the implementation does not change the Slot identity.

---

## Sensor Factory

SensorFactory converts a valid Sensor Slot Configuration into a concrete ISensor instance.

Conceptually:

    Slot 4

    Name:
        Outside

    Implementation:
        AM2302

    GPIO:
        27

            |
            v

        SensorFactory

            |
            v

        AM2302Sensor(
            SensorId = 4,
            GPIO = 27
        )

SensorManager does not construct Sensors.

SensorManager only receives valid runtime instances.

---

## Hardware Interface Kind

Sensor implementations describe their primary hardware interface using a typed classification.

Initial conceptual values include:

- Simulation
- GPIO
- I2C
- OneWire
- ADC
- SPI
- Custom

This classification exists for:

- validation
- configuration UI
- diagnostics
- resource assignment

It does not define Sensor identity.

---

## Custom Protocols

A custom electrical protocol must not be falsely classified as another standardized bus.

Example:

AM2302 / DHT22 uses a proprietary single-data-wire protocol.

It is therefore not classified as Dallas OneWire.

Its interface is represented as:

    GPIO / Custom single-wire protocol

This prevents ambiguity between electrical topology and protocol semantics.

---

## Hardware Resource Model

Hardware resources are explicitly modeled.

Examples include:

- GPIO
- I2C bus
- I2C address
- OneWire bus
- OneWire ROM identity
- ADC channel
- SPI bus
- chip-select GPIO

Resources have type-specific allocation rules.

A single generic resource identifier is not sufficient.

---

## GPIO Assignment

GPIO-based Sensors receive an explicitly configured GPIO or board-level digital port.

Example:

    AM2302
        GPIO = 27

GPIO assignment must be validated before Sensor construction.

Validation includes:

- GPIO exists on the target board
- GPIO is allowed for general Sensor use
- GPIO is not reserved by firmware infrastructure
- GPIO is not already assigned exclusively to another incompatible Sensor
- GPIO supports the required direction/capability

Bootstrapping, flash, UART or otherwise unsuitable pins may be excluded from the selectable resource set.

---

## I2C Assignment

I2C Sensors share a bus.

The resource model therefore distinguishes:

- I2C bus
- device address

Example:

    I2C Bus 0
        SDA = GPIO21
        SCL = GPIO22

        SHT4x   address 0x44
        BMP390  address 0x77
        ADS1115 address 0x48
        AS5600  address 0x36

Sharing one I2C bus is valid.

Two enabled Sensors using the same address on the same bus are invalid unless the hardware explicitly supports address multiplexing.

---

## OneWire Assignment

Dallas OneWire Sensors share one bus.

Configuration may include:

- OneWire bus
- optional or required ROM address

Multiple Sensors may use the same OneWire bus.

When more than one matching device exists, stable device identity may require ROM selection.

The exact policy is implementation-specific.

---

## ADC Assignment

Analog Sensors use explicit ADC resources.

Examples include:

- ADC channel
- external ADC device
- differential input pair

Resource validation prevents conflicting exclusive channel assignments.

Raw ADC representation remains hardware-level implementation detail.

Primary Measurements remain canonical as defined by ADR-0005.

---

## SPI Assignment

SPI Sensors may share a bus.

Each device typically requires an independent chip-select resource.

Configuration therefore distinguishes:

- SPI bus
- chip-select GPIO
- implementation-specific bus parameters where required

SPI support may be introduced only when a concrete Sensor requires it.

---

## Board Capabilities

The Device may expose a board-specific capability description.

Conceptually:

    BoardCapabilities

        I2C Bus 0
            SDA GPIO21
            SCL GPIO22

        Digital Port 1
            GPIO27

        Digital Port 2
            GPIO26

        Digital Port 3
            GPIO25

        OneWire Port 1
            GPIO33

The Web Interface may present logical connector names instead of raw MCU pins.

Example:

    Digital Port 1

rather than:

    GPIO27

The raw GPIO may still be shown as diagnostic detail.

This separates user configuration from PCB implementation details.

---

## Resource Reservation

Some hardware resources belong to infrastructure rather than configurable Sensors.

Examples may include:

- I2C default pins
- serial console pins
- flash-related pins
- future OTA or board functions
- actuator outputs

These resources are marked reserved and are not presented as general Sensor resources.

Resource reservation is board-specific.

---

## Resource Conflict Validation

Sensor Slot configuration must be validated before runtime construction.

Examples of invalid configuration:

    Slot 4
        AM2302
        GPIO27

    Slot 5
        RainGauge
        GPIO27

if both require exclusive use.

Example of valid configuration:

    Slot 4
        SHT4x
        I2C0 / 0x44

    Slot 5
        BMP390
        I2C0 / 0x77

Resource validation follows interface-specific rules.

---

## Validation Boundary

ConfigurationService remains the authoritative persistence owner.

Sensor configuration validation may use dedicated registry/resource helpers.

Validation includes:

- recognized implementation
- valid SlotId
- unique SlotId
- valid hardware resource
- resource compatibility
- resource collision detection
- valid implementation-specific parameters
- valid schedule

Invalid configuration must not result in construction of an undefined Sensor.

---

## Implementation-Specific Configuration

Common Sensor Slot configuration remains small.

Implementation-specific configuration remains strongly typed.

Examples:

    AM2302Configuration
        GPIO

    SHT4xConfiguration
        I2C bus
        address

    SHTC3Configuration
        I2C bus
        fixed address 0x70

    BMP390Configuration
        I2C bus
        address
        oversampling
        filter

    RainGaugeConfiguration
        GPIO
        debounce

Do not use a generic string key/value property bag as the default implementation model.

---

## Default Configuration

Each Sensor implementation may define sensible defaults.

Examples:

AM2302:

    sample interval = 5000 ms

SHT4x:

    default I2C address = 0x44

SHTC3:

    fixed I2C address = 0x70

Defaults assist configuration but do not override explicit user settings.

Invalid explicit configuration does not silently select unrelated resources.

---

## Physical Detection

Hardware assignment and hardware detection remain separate.

Example:

    Slot 4
        Implementation: SHT4x
        Bus: I2C0
        Address: 0x44

Runtime:

    Detected: No
    State: Failed

This is valid.

Detection never silently changes Sensor configuration.

---

## Sensor Selection UI

The Web Interface may generate Sensor configuration controls using registry metadata.

Conceptually:

    Slot 4

    Name:
        Outside

    Implementation:
        AM2302 / DHT22

    Interface:
        Digital GPIO / Custom

    Connection:
        Digital Port 1 / GPIO27

    Measurements:
        Temperature
        RelativeHumidity

    Schedule:
        5000 ms

    Enabled:
        Yes

The implementation selector only shows Sensor types compiled into the current firmware.

---

## Progressive Disclosure

Common configuration remains simple.

Normal view may show:

- Slot
- Name
- Enabled
- Implementation
- Connection
- Interval

Advanced view may expose:

- I2C address
- oversampling
- filtering
- debounce
- calibration
- other implementation-specific settings

Developer view may expose:

- raw GPIO numbers
- bus diagnostics
- detection details

This follows ADR-0007.

---

## Runtime Composition

At startup:

    ConfigurationService
            |
            v
    Sensor Slot Configurations
            |
            v
      Resource Validation
            |
            v
        SensorFactory
            |
            v
       ISensor instances
            |
            v
       SensorManager

Only enabled, valid Sensor Slots participate in runtime composition.

---

## Runtime Reconfiguration

Changing Sensor Slot configuration may require:

    RestartSensorManager

as defined by ADR-0009.

Dynamic Sensor reconstruction may be implemented later.

The configuration model does not require a complete Device restart.

---

## Responsibilities

### Sensor Implementation Registry

Owns:

- available implementation metadata
- implementation identifiers
- capability descriptions
- default configuration metadata

Does not own:

- persistent configuration
- runtime Sensor instances

---

### ConfigurationService

Owns:

- Sensor Slot persistence
- desired Device composition
- configuration validation lifecycle

Does not construct Sensors.

---

### Hardware Resource Model

Owns:

- available board resources
- resource capabilities
- reservation information
- conflict validation rules

---

### SensorFactory

Owns:

- mapping implementation selection to concrete ISensor construction

Does not own:

- Sensor scheduling
- persistence
- MQTT

---

### SensorManager

Owns:

- runtime Sensor orchestration

It does not decide:

- which implementation exists
- which hardware resource is assigned
- how configuration is persisted

---

### Web Interface

Owns:

- presentation and editing of Sensor Slot configuration

It does not:

- invent implementation capabilities
- validate hardware conflicts independently
- construct Sensors

---

## Consequences

New Sensor implementations become selectable without changing SensorManager.

The Device can be composed through configuration rather than source-code modification.

Hardware conflicts can be detected before runtime.

The same firmware can support multiple hardware installations.

Board-level connector abstractions may hide raw ESP32 GPIO details.

Sensor implementation remains strongly typed.

Future physical Sensor integration requires:

- driver implementation
- implementation metadata
- strongly typed configuration
- SensorFactory construction support

No changes to Measurement, MQTT or scheduling architecture are required.

---

## Alternatives Considered

### Hard-code all Sensors in main.cpp

Rejected as the long-term model.

It requires source changes for every installation.

---

### Allow arbitrary GPIO numbers without validation

Rejected.

This permits unsafe or conflicting hardware assignments.

---

### Treat every hardware interface as a generic port

Rejected.

GPIO, I2C, OneWire, ADC and SPI have different sharing and conflict semantics.

---

### Store implementation configuration as free-form key/value pairs

Rejected.

This weakens compile-time safety and makes validation difficult.

---

### Auto-detect all hardware and instantiate it automatically

Rejected.

Detected hardware does not define desired Device composition.

Detection assists Diagnostics and future setup workflows only.

---

### Let SensorManager construct Sensors

Rejected.

Construction and orchestration are separate responsibilities.

---

## Design Rule

The Registry describes what the firmware can do.

Sensor Slots describe what this Device should do.

Hardware Resources describe what the board can provide.

SensorFactory builds the requested runtime composition.

SensorManager only operates the resulting Sensors.
