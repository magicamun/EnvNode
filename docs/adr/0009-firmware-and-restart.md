# ADR-0009

# Firmware Update and Runtime Restart Model

- Status: Accepted
- Date: 2026-08-07

---

## Context

WeatherStation contains multiple long-running subsystems with different restart and reconfiguration requirements.

Examples include:

- WiFi
- MQTT
- Time synchronization
- Sensors
- SensorManager
- Web administration
- firmware update

Some configuration changes can become active immediately.

Others require only one subsystem to be restarted or reinitialized.

A small number of changes require a complete Device restart.

Firmware updates also require an explicit activation step and restart.

Without a centralized runtime lifecycle model, restart behaviour could become distributed across:

- WebService
- ConfigurationService
- OTA implementation
- individual infrastructure services
- Sensor code

This would lead to hidden `ESP.restart()` calls, inconsistent user experience and difficult-to-reason-about runtime behaviour.

WeatherStation therefore requires one explicit model for:

- runtime effects of configuration changes
- subsystem restart requests
- Device restart requests
- firmware update activation
- restart coordination

---

## Decision

WeatherStation introduces a dedicated runtime lifecycle responsibility.

Conceptually:

    Configuration Change
            |
            v
       Runtime Effect
            |
            v
      RuntimeManager
            |
            +----> no runtime action
            |
            +----> restart subsystem
            |
            +----> restart Device

Firmware update uses the same restart model.

No subsystem performs an uncontrolled Device restart directly.

---

## Runtime Effects

Every configuration change has one explicit runtime effect.

Initial runtime effects include:

- Immediate
- RestartMqtt
- RestartTime
- RestartWiFi
- RestartSensorManager
- RestartDevice

## Implementation status (2026-08-17)

Implemented. The runtime-effect set now also includes `RestartActuatorRuntime` and `RestartControllerRuntime`. Sensor, Actuator and Controller configuration can be applied through their separate live rebuild paths without a full Device restart. `RuntimeManager` remains the lifecycle boundary and does not merge these domain runtimes into a generic manager.

The exact enum names are implementation details.

The architectural principle is that configuration changes declare their required runtime effect rather than performing the effect themselves.

---

## Immediate Changes

Immediate changes become effective without restarting a subsystem or the Device.

Examples include:

- Presentation Unit changes
- Locale changes
- future logging preferences
- future display preferences

Conceptually:

    Save Configuration
           |
           v
       Validate
           |
           v
        Persist
           |
           v
    Update Runtime State
           |
           v
        Continue

No restart occurs.

---

## Subsystem Restart

Some changes require one subsystem to be cleanly reconfigured.

Examples include:

MQTT configuration:

- broker
- port
- username
- password

may require:

    RestartMqtt

Time configuration:

- NTP servers
- timezone-related synchronization settings

may require:

    RestartTime

Sensor configuration:

- implementation
- schedule
- enablement
- implementation-specific settings

may require:

    RestartSensorManager

A subsystem restart must not require a complete Device restart unless the subsystem architecture cannot safely support runtime reconfiguration.

The implementation may initially use a broader restart where necessary, but the runtime effect model remains explicit.

---

## Device Restart

A complete Device restart is reserved for changes that genuinely require platform reinitialization.

Examples include:

- firmware activation after OTA
- factory reset
- selected network configuration changes where the current WiFi stack requires full restart
- other future platform-level changes

Device restart is therefore an explicit lifecycle action rather than an incidental side effect of saving configuration.

---

## RuntimeManager

RuntimeManager owns execution and coordination of runtime actions.

Its responsibilities include:

- accepting runtime action requests
- coordinating subsystem restart requests
- coordinating Device restart requests
- preventing conflicting lifecycle operations
- exposing pending runtime action state
- executing actions at a safe point in the application runtime

Conceptually:

    requester
        |
        v
    RuntimeManager.request(...)
        |
        v
    pending Runtime Action
        |
        v
    RuntimeManager.service()
        |
        v
    execute safe action

RuntimeManager does not own configuration.

RuntimeManager reacts to declared runtime effects.

---

## RuntimeManager Boundary

Subsystems request actions.

They do not perform lifecycle actions outside their responsibility.

Examples:

ConfigurationService

    configuration persisted
          |
          v
    RuntimeEffect returned

WebService

    presents resulting effect

OTAService

    firmware successfully staged
          |
          v
    request RestartDevice

No component except RuntimeManager performs an uncontrolled platform restart.

---

## Device Restart Ownership

Direct calls to:

    ESP.restart()

outside the dedicated runtime lifecycle implementation are prohibited.

All Device restart requests go through RuntimeManager.

This provides one authoritative restart boundary.

---

## Restart Request Versus Restart Execution

Requesting a restart and executing a restart are separate operations.

Example:

    Configuration saved
          |
          v
    RestartDevice required
          |
          v
    RuntimeManager stores pending action
          |
          v
    User selects Restart Now
          |
          v
    RuntimeManager executes restart

This distinction allows the Web Interface to communicate restart requirements clearly.

---

## User Experience

The Web Interface must never make restart behaviour mysterious.

After configuration changes it should clearly communicate the runtime effect.

Examples:

Immediate:

    Configuration saved.

    Changes are active.

Subsystem restart:

    Configuration saved.

    MQTT is reconnecting.

Device restart required:

    Configuration saved.

    A Device restart is required.

    [Restart now]
    [Restart later]

The exact wording is a presentation concern.

The lifecycle semantics remain defined by RuntimeManager.

---

## Restart Later

A Device restart may remain pending.

The Device may continue operating with the previous active runtime configuration where technically required.

The Web Interface must be able to indicate:

    Restart required

until the pending action has been completed.

A pending restart is runtime state, not persistent configuration.

If the Device is restarted for another reason, the newly persisted configuration becomes active during startup.

---

## Configuration Persistence and Activation

Persistence and runtime activation are distinct.

Conceptually:

    New Configuration
          |
          v
       Validate
          |
          v
       Persist
          |
          v
    Determine Runtime Effect
          |
          +---- Immediate
          |
          +---- Restart Subsystem
          |
          +---- Restart Device

Persisted configuration always represents the desired Device configuration.

Runtime state may temporarily still reflect the previously active configuration until the required runtime action has completed.

This difference must be visible where relevant.

---

## Runtime Effect Combination

One configuration request may change multiple settings.

The resulting runtime effect is the strongest required action.

Conceptually:

    Immediate
        <
    RestartMqtt
        <
    RestartTime
        <
    RestartWiFi
        <
    RestartSensorManager
        <
    RestartDevice

The exact ordering between independent subsystem restarts is an implementation detail.

The important rule is:

If any changed setting requires RestartDevice, the complete result requires RestartDevice.

If multiple independent subsystem restarts are required and Device restart is not required, RuntimeManager may execute each required subsystem action.

Runtime effects must never be silently lost.

---

## Firmware Update Model

Firmware update is a staged lifecycle process.

Conceptually:

    Running
       |
       v
    Uploading
       |
       v
    Verifying
       |
       v
    Firmware Staged
       |
       v
    Restart Required
       |
       v
    Restarting
       |
       v
    Starting
       |
       v
    Running New Firmware

Firmware upload does not immediately execute new firmware.

Activation occurs after Device restart.

---

## OTAService

OTAService owns firmware-update mechanics.

Its responsibilities include:

- receiving firmware image data
- writing the image to the OTA target partition
- validating write success
- reporting update progress
- reporting update result
- marking a successfully written image ready for activation

OTAService does not own:

- Web navigation
- Configuration
- MQTT
- Sensor operation
- Device restart execution

After successful staging, OTAService requests:

    RestartDevice

through RuntimeManager.

---

## OTA Failure

A failed update must leave the currently running firmware operational.

Examples include:

- upload interrupted
- invalid image
- flash write failure
- verification failure

Failure does not trigger automatic Device restart.

The existing firmware remains active.

Diagnostics report the failure.

---

## Firmware Verification

The update path must reject images that cannot be safely activated.

Verification may include:

- firmware image format validity
- flash write success
- partition compatibility
- future project-specific metadata validation

The exact verification mechanism depends on platform capabilities.

A successful HTTP upload alone is not sufficient evidence of successful firmware staging.

---

## OTA Activation

A staged firmware image becomes active only after restart.

The Web Interface should communicate:

    Firmware uploaded successfully.

    Restart required to activate.

The user may choose:

    Restart now

or, where supported:

    Restart later

No hidden restart should occur solely because the upload request completed unless explicitly defined by a future policy.

---

## Firmware Version

The currently running firmware version and the newly uploaded firmware version are different concepts.

Where metadata allows it, firmware administration should distinguish:

- running version
- staged version

The firmware version remains defined by the project's authoritative version source.

OTA does not introduce a second version authority.

---

## Runtime State

Runtime lifecycle state is distinct from configuration.

Examples include:

- Running
- FirmwareUploadInProgress
- FirmwareStaged
- RestartPending
- Restarting

The exact state-machine representation may remain lightweight.

The architecture does not require an elaborate general-purpose workflow engine.

---

## Conflicting Runtime Actions

Conflicting lifecycle actions must be controlled.

Examples:

During firmware upload:

- Device restart must not occur
- factory reset must not occur
- another OTA upload must not begin

During Device restart preparation:

- new lifecycle actions may be rejected

RuntimeManager and OTAService cooperate to preserve safe transitions.

---

## Factory Reset

Factory reset follows the same lifecycle model.

Conceptually:

    User confirms Factory Reset
          |
          v
    ConfigurationService clears persistent configuration
          |
          v
    RuntimeManager requests RestartDevice
          |
          v
    Device restarts
          |
          v
    Default Configuration
          |
          v
    Provisioning

WebService never calls `ESP.restart()` directly.

---

## Network Changes

Network changes may temporarily make the current Web connection invalid.

Examples include:

- SSID
- WiFi password
- hostname
- DHCP / Static addressing
- IP address
- subnet
- gateway
- DNS

For the initial implementation, these settings may require:

    RestartDevice

This is acceptable even if a future WiFiService can reconfigure them dynamically.

The architecture permits later refinement to:

    RestartWiFi

without changing the configuration model or Web UI semantics.

---

## MQTT Changes

MQTT settings conceptually require:

    RestartMqtt

rather than:

    RestartDevice

The initial implementation may still use Device restart until clean runtime reconfiguration exists.

This is an implementation limitation, not the intended architectural model.

---

## Time Changes

Time/NTP changes conceptually require:

    RestartTime

Timezone display changes that do not require reinitializing synchronization may eventually be applied immediately.

The exact mapping is owned by the configuration/runtime-effect definition.

WebService must not decide this ad hoc.

---

## Sensor Changes

Sensor Slot changes conceptually require Sensor composition to be rebuilt.

Examples include:

- enabling a Slot
- disabling a Slot
- selecting a different implementation
- changing hardware-specific configuration

The expected runtime effect is:

    RestartSensorManager

or an equivalent future Sensor runtime reconfiguration action.

SensorManager itself does not persist configuration.

SensorFactory reconstructs configured Sensor instances at the appropriate lifecycle boundary.

---

## Web Administration

The Firmware page provides firmware lifecycle administration.

Typical information includes:

- running firmware version
- firmware upload
- upload progress
- firmware staging result
- pending restart
- Restart Now

The Device/Firmware administration interface may also expose explicit:

- Restart
- Factory Reset

All actions use RuntimeManager.

---

## Progress Reporting

Firmware upload progress is runtime state.

It may be exposed as:

- bytes received
- total bytes
- percentage
- current update state

Progress reporting must not require historical persistence.

A page reload may reconstruct the current state through OTAService runtime status where technically possible.

---

## Application Integration

Application remains the composition and orchestration boundary.

Conceptually:

    Application
        |
        +---- ConfigurationService
        +---- WiFiService
        +---- TimeService
        +---- MqttService
        +---- SensorManager
        +---- MeasurementPublisher
        +---- WebService
        +---- OTAService
        +---- RuntimeManager

RuntimeManager coordinates lifecycle effects but does not replace Application orchestration.

---

## Failure Isolation

Lifecycle management must preserve the existing failure-isolation rules.

Examples:

MQTT restart:

- Sensor acquisition continues
- Time remains synchronized
- Web remains operational

Time restart:

- Sensor acquisition continues
- MQTT connectivity remains available
- Measurement publication pauses where valid time is required

SensorManager restart:

- infrastructure remains operational
- unrelated communication services continue

Firmware update failure:

- currently running firmware remains operational

Only RestartDevice intentionally resets the complete runtime.

---

## Consequences

Restart behaviour becomes explicit and predictable.

Configuration saving no longer implies automatic Device reboot.

Subsystem-specific reconfiguration can evolve independently.

Firmware update uses the same runtime-action model as configuration changes.

Direct scattered calls to `ESP.restart()` disappear.

The Web Interface can clearly communicate why a restart is required.

Future runtime reconfiguration can reduce the number of full Device restarts without changing the public administration model.

---

## Alternatives Considered

### Restart the Device after every configuration save

Rejected as the architectural model.

It is simple but causes unnecessary interruption and makes configuration behaviour unintuitive.

---

### Let every subsystem restart itself

Rejected.

This distributes lifecycle policy across unrelated components and makes conflicts difficult to coordinate.

---

### Let WebService directly restart subsystems

Rejected.

WebService is an administration adapter, not the runtime lifecycle owner.

---

### Let OTAService call ESP.restart() directly

Rejected.

Firmware update determines that activation requires restart.

RuntimeManager owns restart execution.

---

### Apply every configuration change dynamically

Rejected.

Some platform changes legitimately require restart, and forcing runtime reconfiguration increases complexity without corresponding value.

---

### Always restart automatically after OTA upload

Rejected as the default policy.

Successful staging and activation are separate lifecycle events.

The user or a future explicit policy determines when activation restart occurs.

---

## Design Rule

Configuration declares desired state.

Runtime effects describe what must change.

RuntimeManager coordinates change.

OTAService stages firmware.

Only RuntimeManager restarts the Device.

A restart is an explicit lifecycle action, never a hidden side effect.
