# ADR-0006

# Device Configuration Model

- Status: Accepted
- Date: 2026-08-07

---

## Context

WeatherStation configuration has grown beyond a small set of independent device parameters.

The current firmware already contains configuration for:

- device identity
- WiFi
- MQTT
- time and NTP
- presentation units

Future functionality adds configuration for:

- physical and simulated Sensors
- Sensor enablement
- Sensor implementation selection
- acquisition schedules
- Sensor-specific calibration and filtering
- network addressing mode
- diagnostics
- firmware administration

A single flat configuration object can technically hold these values, but it does not express their ownership or lifecycle clearly.

The administration interface also requires configuration to be presented in meaningful functional areas rather than as one undifferentiated form.

WeatherStation therefore requires a structured configuration model that separates configuration by responsibility while preserving one authoritative configuration owner.

---

## Decision

WeatherStation uses one authoritative Device Configuration composed of functional configuration sections.

Conceptually:

    Configuration
        |
        +---- DeviceConfiguration
        |
        +---- NetworkConfiguration
        |
        +---- MqttConfiguration
        |
        +---- TimeConfiguration
        |
        +---- PresentationConfiguration
        |
        +---- SensorConfiguration[]
        |
        +---- DiagnosticsConfiguration
        |
        +---- future configuration sections

ConfigurationService remains the single authoritative owner of persistent configuration.

The configuration sections describe logical responsibility only.

They do not become independent persistence owners.

---

## Configuration Ownership

ConfigurationService owns:

- loading persistent configuration
- applying defaults
- validation
- persistence
- runtime configuration state
- reset-to-default behaviour

No individual subsystem persists its own configuration directly.

Examples:

- WiFiService consumes NetworkConfiguration
- MqttService consumes MqttConfiguration
- TimeService consumes TimeConfiguration
- MeasurementPublisher consumes PresentationConfiguration
- Sensor creation consumes SensorConfiguration

Conceptually:

    Persistent Storage
           |
           v
    ConfigurationService
           |
           v
      Configuration
           |
       +---+---+---+---+---+
       |   |   |   |   |   |
       v   v   v   v   v   v
     WiFi MQTT Time Sensors Publisher Web

There is still exactly one configuration source of truth.

---

## Device Configuration

DeviceConfiguration contains general Device identity and behaviour that does not belong to a more specific subsystem.

Examples include:

- device name
- future installation description
- future location metadata
- future global operating options

DeviceConfiguration must not become a generic container for unrelated settings.

Configuration belongs to the most specific functional section available.

---

## Network Configuration

NetworkConfiguration contains settings required to establish IP network connectivity.

It includes:

- WiFi SSID
- WiFi password
- hostname
- address mode

Address mode supports:

- DHCP
- Static

DHCP is the default.

When Static is selected, NetworkConfiguration additionally contains:

- IPv4 address
- subnet mask
- default gateway
- primary DNS server
- secondary DNS server where configured

Static addressing is optional.

A valid DHCP configuration does not require static-address fields.

A valid Static configuration requires a complete and internally consistent set of required addressing fields.

---

## Network Configuration Lifecycle

Network configuration affects the underlying IP stack.

Changes to network settings are therefore applied on Device restart.

Examples include:

- WiFi SSID
- WiFi password
- hostname
- DHCP / Static mode
- IP address
- subnet mask
- gateway
- DNS servers

The administration interface must clearly communicate that these changes require restart.

Network configuration is separated from settings that can be applied without restarting the Device.

This distinction is a lifecycle concern, not a separate persistence model.

---

## MQTT Configuration

MqttConfiguration contains broker integration settings.

Examples include:

- server
- port
- username
- password

Future MQTT-specific options may include:

- topic prefix
- availability behaviour
- publish policy

MQTT configuration remains independent from Measurement semantics.

Presentation units and Sensor schedules do not belong to MqttConfiguration.

Changes that require reconnection may initially continue to use the existing restart-based configuration workflow.

Dynamic reconnection may be introduced later without changing the configuration model.

---

## Time Configuration

TimeConfiguration contains:

- POSIX timezone
- NTP server configuration

It does not contain runtime synchronization state.

Examples of runtime state that are not Configuration:

- synchronized
- synchronization pending
- last successful synchronization
- current Unix epoch

Those belong to runtime state and Diagnostics.

---

## Presentation Configuration

PresentationConfiguration contains the globally selected Presentation Unit for each configurable MeasurementType as defined by ADR-0005.

Examples include:

    Temperature -> DegreeCelsius

    AtmosphericPressure -> Hectopascal

PresentationConfiguration affects external representation only.

It does not change:

- Sensor acquisition
- canonical Measurement values
- calibration
- scheduling
- Sensor lifecycle

Presentation configuration is global per MeasurementType rather than per Sensor.

---

## Sensor Slot Configuration

SensorSlotConfiguration describes one logical Sensor position within the Device configuration.

A Sensor Slot is independent from the concrete Sensor implementation assigned to it.

Conceptually:

    SensorSlotConfiguration
        |
        +---- SensorId
        +---- enabled
        +---- implementation
        +---- schedule
        +---- implementation-specific configuration

The Slot defines the stable logical identity of a measurement source.

The selected implementation defines how that logical source is realized.

Example:

    Slot 1
        |
        +---- SHT4x

may later become:

    Slot 1
        |
        +---- SimulatedTemperature

or another compatible implementation without changing the identity of the Slot.

SensorManager does not depend on the concrete implementation.

It receives the Sensor instance created for the configured Slot.

Multiple Sensor Slots may:

- use the same Sensor implementation
- produce the same MeasurementType
- produce overlapping sets of MeasurementTypes

There is intentionally no uniqueness constraint on MeasurementType within one Device.

Measurements remain distinguishable through their source SensorId.

For example:

    Slot 1 -> SHT4x
                +---- Temperature
                +---- RelativeHumidity

    Slot 2 -> BMP390
                +---- Temperature
                +---- AtmosphericPressure

    Slot 3 -> DS18B20
                +---- Temperature

All three Temperature Measurements are valid and remain distinguishable by source.

---

## Hardware Resource Compatibility

Sensor implementation selection is constrained by the physical resources required by the configured hardware.

Examples include:

- I2C bus and address
- SPI bus and chip-select
- GPIO assignment
- ADC input
- OneWire bus and device identity

Multiple Sensor Slots may use identical Sensor implementations when their hardware configuration allows them to coexist.

Likewise, multiple Sensors may produce identical MeasurementTypes.

Neither SensorImplementation nor MeasurementType is globally unique.

Hardware-resource conflicts are configuration validation concerns.

Examples of invalid configurations may include:

- two exclusive devices assigned to the same GPIO
- two I2C devices with the same fixed address on the same bus when no hardware multiplexing exists
- conflicting ADC channel assignments

Examples of valid configurations may include:

- multiple I2C Sensors using different addresses
- multiple identical Sensors on different buses
- multiple OneWire Sensors on one compatible bus
- multiple Sensors producing Temperature Measurements

The architecture does not require a generic hardware resource manager at this stage.

Resource compatibility may initially be validated by the relevant Sensor implementation or SensorFactory using implementation-specific configuration rules.

Hardware conflict validation must not introduce uniqueness constraints on logical Sensor Slots or MeasurementTypes.

---

## Sensor Identity

Every configured Sensor has a stable logical SensorId.

Rules remain consistent with the domain model:

- SensorId is nonzero
- SensorId is unique within one Device
- one SensorId represents one logical measurement source
- one Sensor may produce multiple MeasurementTypes

Sensor identity belongs to configuration when selecting the logical Device composition.

Concrete Sensor instances receive that configured identity during construction.

---

## Sensor Enablement

Each SensorConfiguration contains an explicit enabled state.

Disabled Sensors:

- are not initialized
- are not serviced
- are not periodically sampled
- do not produce Measurements

Whether a disabled Sensor instance is constructed internally is an implementation detail.

From the runtime point of view, disabled Sensors do not participate in normal SensorManager operation.

---

## Sensor Implementation Selection

SensorConfiguration contains a typed Sensor implementation selection.

Conceptually:

    SensorImplementation

        None

        SimulatedTemperature
        SimulatedHumidity
        SimulatedPressure

        SHT4x
        BMP390

        future implementations

The exact list evolves as Sensor drivers are added.

Free-form class names or arbitrary strings are not used as authoritative implementation identity.

A typed implementation identifier allows:

- validation
- Web Interface dropdown generation
- predictable Sensor construction
- stable persistent configuration

---

## Sensor Factory Boundary

Each enabled Sensor Slot is converted into a concrete Sensor instance at a dedicated construction boundary.

Conceptually:

    SensorSlotConfiguration
            |
            v
       SensorFactory
            |
            +----> SimulatedTemperatureSensor
            |
            +----> SHT4xSensor
            |
            +----> BMP390Sensor
            |
            +----> ...

SensorFactory interprets the selected SensorImplementation and its implementation-specific configuration.

It constructs the concrete Sensor with the stable SensorId belonging to the Slot.

SensorManager does not construct Sensors and does not know concrete Sensor implementations.

SensorManager only receives configured Sensor instances through registration.

This preserves the separation between:

- persistent configuration
- Sensor construction
- hardware-specific configuration
- Sensor orchestration

The same concrete Sensor implementation may be instantiated for multiple Slots when the underlying hardware configuration permits it.

SensorFactory is also the appropriate boundary for rejecting impossible or conflicting hardware configurations before Sensor registration.

---

## Sensor Physical Availability

Configured Sensor implementation and physical Sensor availability are separate concepts.

Example:

    Configured Implementation: BMP390

    Physical Detection: unavailable

This is a valid configuration.

The Sensor may then enter:

    Initializing
        |
        v
      Failed

and Diagnostics may report that the configured hardware was not detected.

The administration interface should distinguish:

- configured
- enabled
- physically detected
- operational state

Physical auto-detection must not silently replace configuration.

A detected Sensor does not automatically become configured unless an explicit future discovery workflow defines that behaviour.

---

## Sensor Scheduling Configuration

Periodic acquisition configuration belongs to SensorConfiguration.

The scheduling model remains consistent with SensorManager architecture.

Conceptually:

    SensorSchedule
        |
        +---- AcquisitionMode
        |
        +---- sampleInterval

AcquisitionMode supports:

- EventOnly
- Periodic

A periodic Sensor may still emit event-driven output through service().

The configured schedule defines orchestration policy.

Runtime scheduling state such as nextDue or pending sample flags is never persisted.

---

## Sensor-Specific Configuration

Different Sensor implementations may require different parameters.

Examples:

SHT4x

- future precision or heater-related acquisition options

BMP390

- oversampling
- filtering

Rain Detector

- wet threshold
- smoothing parameters
- heater configuration

Simulated Sensor

- simulation amplitude
- period
- offset where configuration is later required

These parameters are implementation-specific.

They must not be forced into one large generic SensorConfiguration containing fields irrelevant to most Sensors.

The architecture therefore permits implementation-specific Sensor configuration.

---

## Implementation-Specific Configuration Boundary

The common SensorConfiguration owns only properties common to all Sensors.

Examples:

- SensorId
- enabled
- implementation
- schedule

Implementation-specific values belong to the selected Sensor implementation configuration.

Conceptually:

    SensorConfiguration
        |
        +---- Common Configuration
        |
        +---- Implementation Configuration
                |
                +---- SHT4xConfiguration
                +---- BMP390Configuration
                +---- RainDetectorConfiguration
                +---- SimulationConfiguration

The exact C++ representation may evolve as concrete Sensor drivers are implemented.

The configuration architecture does not require a single generic property bag.

Strongly typed configuration remains preferred.

---

## Sensor Configuration Validation

ConfigurationService validates persisted Sensor configuration before it becomes active runtime configuration.

Validation includes common invariants such as:

- SensorId is valid
- SensorIds are unique
- implementation identifier is recognized
- acquisition mode is recognized
- periodic interval is valid
- implementation-specific configuration is valid

Invalid Sensor configuration must never result in an unpredictably configured hardware driver.

Fallback behaviour must be explicit.

Depending on the configuration error, appropriate behaviour may include:

- disabling the affected Sensor
- falling back to an implementation default
- reporting a configuration Diagnostic

Invalid Sensor configuration must not prevent unrelated Sensors from operating.

---

## Configuration and Runtime State

Configuration and runtime state are strictly separated.

Examples:

Configuration:

    Sensor implementation = BMP390
    enabled = true
    interval = 30000 ms

Runtime state:

    state = Ready
    detected = true
    last operation = Completed

Configuration:

    MQTT server = mosquitto.example

Runtime state:

    MQTT connected = true

Configuration:

    timezone = Europe/Berlin POSIX rule

Runtime state:

    Time synchronized = true

The Web Interface may display both configuration and runtime state, but they remain different data sources.

---

## Configuration Change Categories

Configuration changes fall into two broad lifecycle categories.

### Restart-required Configuration

Examples include:

- WiFi credentials
- hostname
- DHCP / Static addressing
- IP configuration
- other settings that alter basic platform initialization

These settings are persisted and become active after restart.

### Runtime-applicable Configuration

Examples include:

- Presentation Unit selection
- future diagnostics preferences
- potentially Sensor scheduling parameters

These settings may be applied dynamically when the owning subsystem explicitly supports runtime reconfiguration.

The configuration model does not require every runtime-applicable setting to be applied dynamically immediately.

A restart-based implementation remains valid until explicit runtime reconfiguration support is added.

The Web Interface must clearly communicate whether a changed setting requires restart.

---

## Configuration Persistence

Persistent storage remains an implementation detail of ConfigurationService.

Individual configuration sections do not map directly to independent NVS namespaces by architectural requirement.

The persistence layout may evolve without changing the domain-facing configuration model.

Persisted identifiers must remain stable enough to support configuration upgrades.

Examples include:

- typed enum representations
- explicitly versioned stable values

Presentation labels and UI text must not become authoritative persisted configuration identifiers.

---

## Configuration Evolution

The configuration model is expected to evolve as Sensors and Device capabilities are added.

Evolution must preserve:

- one authoritative ConfigurationService
- typed configuration
- explicit defaults
- validation before use
- separation of Configuration and runtime state
- subsystem responsibility boundaries

Future configuration migration may become necessary when persisted schema changes.

Migration belongs to ConfigurationService and must not leak into individual Sensors or communication components.

---

## Consequences

The Device configuration becomes structurally aligned with the actual firmware architecture.

The Web Interface can present configuration by functional responsibility without becoming the configuration owner.

Adding a new Sensor implementation requires:

- a typed implementation identifier
- a Sensor implementation
- implementation-specific configuration where required
- construction support in SensorFactory

It does not require changes to SensorManager architecture.

Adding Static IP configuration affects NetworkConfiguration and WiFiService only.

It does not affect MQTT, Sensor or Measurement architecture.

Presentation configuration remains independent from Sensor configuration.

Runtime state and diagnostics remain separate from persistent settings.

---

## Alternatives Considered

### Keep one flat Configuration structure indefinitely

Rejected as the long-term model.

A flat structure becomes increasingly difficult to reason about as networking, Sensors, presentation and administration evolve independently.

---

### Let each subsystem persist its own configuration

Rejected.

This would create multiple persistence owners and competing sources of truth.

ConfigurationService remains authoritative.

---

### Store Sensor implementations as arbitrary strings

Rejected.

Free-form implementation identifiers provide weak validation and make configuration migrations and UI generation fragile.

---

### Configure Sensors entirely at compile time

Rejected.

This prevents Device installations from selecting, enabling or replacing Sensor implementations without firmware modification.

---

### Automatically enable every physically detected Sensor

Rejected.

Detection and configuration are different responsibilities.

Automatic discovery may assist administration in the future, but hardware detection must not silently redefine Device configuration.

---

### Put all Sensor-specific parameters into one generic key/value map

Rejected as the default architecture.

It sacrifices type safety and makes validation difficult.

Strongly typed implementation-specific configuration is preferred.

---

## Design Rule

Configuration describes what the Device should be.

Runtime state describes what the Device currently is.

ConfigurationService owns persistent truth.

Subsystems consume only the configuration they require.

Sensor configuration selects implementations.

SensorManager orchestrates instances.

Hardware detection reports reality but never silently rewrites configuration.