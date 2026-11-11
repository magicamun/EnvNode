<!-- EnvNode-owned: see ../LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0). -->
# RainDetector firmware integration

## Composition

The `rain_detector` sensor owns the PCNT frequency input. The separate configured
`gpio_pwm` or `gpio_on_off` actuator owns the heater GPIO. A `rain_detector`
controller selects those two instance IDs and uses the shared `RainDetectorPolicy`.
It resolves copied monotonic frequency snapshots and actuator capabilities on each
service pass; it retains no sensor/actuator driver pointers. Sensor acquisition
completes at most one gate per application loop, and controller service processes
its revision in that same loop. UTC synchronization and MQTT publication are not
prerequisites for local control.

The frequency channel is `rain_detector_frequency`, in Hz. Existing ratio-valued
`rain_detector_level` is unchanged. Local nominal gates remain 250 ms; the sensor's
configured sampling interval controls publication only (default 5000 ms). Every
local gate, including invalid gates, updates the independent local snapshot.
Automatic heater transitions restart acquisition between windows; a manual
actuator override disarms this controller and also restarts the gate safely.

The sensor and controller factories retain fixed maximum slot capacities, but
allocate their aligned storage arrays once using checked allocation instead of
placing the largest new implementation into every static DRAM slot. The larger
RainDetector history is allocated only for configured RainDetector controllers;
ordinary controllers keep their previous slot size in active and staging pools. There is no
allocation in frequency acquisition or policy evaluation. PCNT unit exhaustion
and driver errors affect only that input; driver acquisition retries once per
second, never in a blocking wait.

## Configuration and activation

1. In Sensors, configure an enabled **RainDetector** with its frequency GPIO.
   Save and apply the sensor runtime. Weather hardware uses GPIO27 in the tested
   wiring; GPIO availability is governed by the device's provisioned board profile.
2. In Actuators, configure an enabled **GPIO PWM** (or GPIO On/Off) actuator on
   the heater-enable pin. Save and apply. GPIO16 is the tested Weather wiring.
   The automatic policy uses only OFF and full ON, including with a PWM actuator.
3. In Controllers, configure an enabled **RainDetector**, selecting that sensor
   and heater actuator. Save and apply. One enabled controller owns the output;
   one RainDetector controller may use a given frequency sensor.
4. Enabled sensors and controllers start automatically after boot or runtime
   activation, just like the other implementations. Wait at least six seconds
   before the first spray. The controller starts with heating off and waits for
   valid signal and the five-second guard. Its one-time startup check then requests
   100% heating below the dry threshold; at/above the threshold it confirms dry
   with heating off for the configured dry duration. Neither path creates rain. Manual Stop disarms it until Start or the next
   boot/runtime activation. A separate configurable autostart option is deferred.

All seven policy settings persist in NVS and configuration backup:

| Setting | Default | Accepted range |
| --- | --- | --- |
| Edge window | 1000 ms | 250–2000 ms |
| Absolute frequency fall | 150 Hz | 20–10000 Hz |
| Relative frequency fall | 5 per mille (0.5%) | 1–200 per mille |
| Rain-end quiet time | 60 s | 30–300 s |
| Dry threshold | 32000 Hz | 1000–100000 Hz |
| Dry hysteresis | 500 Hz | 1 Hz to threshold minus 1 Hz |
| Dry confirmation | 10 s | 1–120 s |

Both fall thresholds must be satisfied. Confirmation begins at the dry threshold,
continues within the hysteresis band, and resets below the lower limit or on new
wetting. Dry confirmation applies inside a startup- or wetting-started 100%
heating cycle, plus the one-time unheated startup check above the dry threshold.
If frequency falls below the threshold before that startup confirmation completes,
initialization switches to full drying heat. An event cooldown of two seconds and heater guard of five seconds retain
the lab behavior. A 250-ms edge window can be too short at actual gate spacing;
the default is recommended for initial testing. These are experimental limits,
not calibrated specifications of rain intensity or surface moisture.

Optional temperature and humidity source selections are persisted and validated
against the selected sensors' capabilities. They are reserved inputs: no dew-point,
cold-weather or proportional preventive heating is active. They are not required
for the current wetting/drying policy. Shared SD diagnostic recording includes every local frequency gate and controller
state/output changes for field analysis (see below).

## Failure and stop behavior

- New wetting starts 100% heating and marks rain active.
- No newly detected wetting for the configured valid quiet time estimates rain end;
  heating remains independent until dry confirmation.
- Zero pulses, overflow, stale/missing source or invalid gates abort heating and
  reset rain to unknown. The started controller remains armed; after signal
  recovery and its guard, only new wetting can restart heating once the startup
  check has been consumed. Invalid data before the initial classification merely
  keep startup pending; they never request heat.
- Driver failure is retried; a temporarily unavailable target is resumed safely
  off when available. Manual output changes, including a partial PWM level while logically ON,
  disarm the controller and command the output off. Start is then
  required again. Stop commands output off and disarms the controller.
- Rain-end and dry are local estimates. Dry latches after the startup check or
  the last heating cycle; a later slow frequency decrease after completed startup
  does not create another initialization or heating cycle. Explicit Start/rebuild
  starts a new initialization; ordinary signal recovery does not.

## Diagnostics and MQTT

With UDP logging configured at `/network-logging`, enable **Extended diagnostics** on the RainDetector
sensor and on its controller. The sensor records every completed measurement gate
(normally 250 ms), including pulse count, frequency and faults. The controller
records configuration, decisions and output changes, with a fresh context every
60 seconds. Records go to the configured rsyslog receiver as JSON; sensor ID and revision
correlate measurements with decisions. See [Diagnostic recording](DiagnosticRecording.md)
for event fields, loss counters and receiver retention. Recording is bounded and best effort;
a full queue drops diagnostic events without delaying measurement or control.
The system-health diagnostics checkbox is independent of these entity selections.

Sensors show current raw frequency and the fault name (`none`, `no_signal`,
`counter_overflow`, `invalid_window`, `driver_error`). A fault does not uniquely
identify a disconnected cable/head. The controller page shows rain, validity,
heating-cycle state, dry completion and detected-wetting count. The existing
Start/Stop controls and MQTT commands are used; lab console commands are not part
of production firmware.

The retained controller status adds `rain`, `valid`, `heating`, `dry` and
`wetting_count`; `running` and `target_available` remain explicit. The count resets
on Start/rebuild and allows consumers to observe further wetting while rain is
already active. Raw Hz uses the normal measurement channel and publication rate.
Policy parameters are published under the existing controller parameter topics;
commands use `cmd/parameter/<name>` with these names: `edge_ms`, `drop_hz`,
`drop_permille`, `end_seconds`, `dry_hz`, `hysteresis_hz`, `dry_seconds`.
A parameter change saves configuration and rebuilds controllers; enabled
RainDetector controllers automatically resume observation with heating off. Retained authoritative parameter state is never a
command subscription.

## First hardware acceptance

Use the existing build/upload workflow for the normal firmware; flashing replaces
the standalone lab image. Preserve the correct board identity and existing NVS
configuration. Configure and apply the three instances above, then verify a
wetting/heating cycle, a 60-second quiet rain end and dry shutdown. Repeat wetting
after cooling. Check Stop, reset, head disconnection and signal recovery using the
same cases validated in the lab. Confirm stored parameters after reboot. These
production hardware checks remain outstanding despite host tests and successful
builds. Full signal polarity, thermal behavior and thresholds must be rechecked
with the modified Head PCB before field use.

Runtime storage pools are allocated on first composition/use, never in global
constructors. This leaves early ESP32 heap available for the FreeRTOS main task
before startup-stack memory becomes available to the normal runtime heap.

The sensor form distinguishes the Rain Detector (wetting/frequency) from the
Rain Gauge (tipping bucket). Implementation metadata follows the current form
selection immediately. The recommended 5000-ms sample interval controls published
frequency readings, independently of the approximately 250-ms local acquisition.

For estimated rain end, the controller page shows the last detected wetting and
the later rain-end detection time separately. These are volatile runtime events,
reset by controller restart; local wall-clock times are derived from monotonic
event ages when time is synchronized, otherwise elapsed ages are shown.
RainDetector diagnostics report measurement usability and actuator availability.

## Frequency presentation and controller properties

The Units page offers Hz and kHz. The selection persists in NVS and is included
in configuration backup/record version 6; stable versions 1–5 remain readable and use Hz.
Only presentation values are converted. PCNT input, thresholds and policy stay in Hz.
Display format unit text remains literal: use e.g. `%5.2f kHz` after choosing kHz.

The display source inventory exposes `controller/<id>/rain` (enum: `unknown`,
`active`, `end_estimated`), `valid`, `heating`, `heater_percent` (unsigned 0/100),
`dry`, `wetting_count`, `running`, and `target_available`. Boolean sources use `%s`
with configurable labels, the count and percent use `%u`. These are controller
status/commands, not physical heater feedback. `valid` describes evaluation validity;
its value remains readable even while the signal is invalid.

Formatting already accepts minimum field widths and float precision: `%3u` prints
42 as ` 42`, `%5.2f` prints 3.5 as ` 3.50`; `%03u` pads with zeros and `%-5.2f`
left-aligns. Width is not truncation. Limits remain 64 characters of width and
0-6 decimal places. Human-facing text uses the configured locale: comma for
German, point for English. MQTT and canonical numeric values retain their
protocol representation.

## Rain event timestamps as display sources

Three additional controller sources use `%s`, like the system date/time sources:

| Property | Meaning | Updated by |
| --- | --- | --- |
| `controller/<id>/rain_start` | Last rain start | First wetting of a new rain episode; later wetting during active rain does not move it |
| `controller/<id>/rain_end` | Last detected wetting (estimated end) | Every newly detected wetting |
| `controller/<id>/rain_end_detected` | Rain-end decision time | Transition to estimated rain end after valid quiet time |

Values are local date/time text in the configured locale. Property snapshots also
carry the event's UTC timestamp in `hasTimestamp`/`timestamp`. Before an event or
before UTC has synchronized they return NoValue, not a fabricated date. Rain
acquisition and heating remain independent of UTC. Monotonic events captured
before synchronization are resolved when the clock becomes available; resolved
UTC times then remain fixed across refreshes and clock corrections. A new event
updates its own value; a new rain episode keeps the previous end-decision time
until a subsequent end is detected. Initialization heating/dry confirmation does
not create rain timestamps. They are volatile: explicit Start/rebuild/reboot
clears them, while Stop or a signal fault retains the last events.

## Startup input availability and composed sensors

Immediately after boot, `no_input` is expected until the first acquisition
window is available. The controller remains armed with heating off, polls again
on each service call, and accepts the first available window automatically;
a restart or artificial startup delay is not required. The normal evaluation
and heater guards still apply before wetting/dry decisions become valid.

A persistent `no_input` despite valid `rain_window` events was traced on
2026-11-05 to the humidity-derived sensor wrapper used for all runtime sensors.
The outer wrapper did not forward the direct frequency-snapshot capability or
heater-change notification, hiding the working RainDetector behind ISensor's
no-input defaults. It now forwards both, like the pressure wrapper already did.
This leaves acquisition timing and thresholds unchanged.

The regression test follows the production composition: RainDetectorSensor →
SeaLevelPressureSensor → HumidityDerivedSensor → SensorManager → rain controller.
It covers empty startup, first-window recovery independent of periodic sample
publication, dry confirmation, wetting/full heat, acquisition restart on heater
change, invalid signal/heater off, and fresh wetting after signal recovery.
The test failed at the first snapshot before the fix and passes with forwarding.
Build/flash and physical validation remain with the user.

## Preventive dew-point heating (binary output)

The RainDetector controller optionally selects **one** enabled temperature and
humidity sensor. Its existing derived `DewPoint` and canonical `Temperature`
channels are read through the measurement resolver. No humidity/dew-point
formula is duplicated in the controller, and no head temperature is inferred.
The actual head surface can be colder or warmer than the protected air sensor.

Source ID 0 (default) disables prevention and preserves the existing rain-only
behavior, including pending laboratory wetting tests. The old optional,
reserved independent temperature/humidity references do not activate this mode.

Configuration uses two gap thresholds in degrees Celsius:

- `gap = air temperature - dew point`.
- `gap <= dewOnC`: latch a preventive heating demand.
- `gap >= dewOffC`: clear that demand.
- Between thresholds, retain the previous demand; startup demand is off.
- Require finite `0 <= dewOnC < dewOffC <= 30`. Initial editable values are
  2 °C / 4 °C, experimental starting points, not field-validated limits.

Prevention applies only after the detector has confirmed dry. Existing startup
classification and rain drying retain priority: wetting/full drying demands
100 %, independent of climate-source availability. When dry is confirmed,
preventive demand can keep the output at 100 % without an off/on pulse. Otherwise
it switches off. A preventive demand never creates a rain event. The displayed
heating status reflects the combined actual software output; `dry=true` and
heating can therefore coexist. All output commands remain exclusively 0/100 %.

Temperature and dew point must be valid, finite, correctly typed/from the selected
source, no older than 120 s, and their acceptance times within 1 s of each other.
This accommodates per-channel timestamp capture within one sensor operation.
Air temperature is restricted to the existing derived-dew calculation range
(-45…60 °C); dew point above air temperature by more than 0.1 °C is rejected.
Missing, stale or invalid climate input clears only the preventive demand.
Detector signal loss/staleness, target loss, Stop and manual override retain the
existing heater-off safety behavior. A detector fault does not autonomously
restart prevention without renewed dry confirmation. The unchanged 5-second
heater-transition guard suppresses direct switching effects on edge detection;
longer thermal effects remain an experimental limitation to observe outdoors.

The web form offers one paired-capability source and localized decimal inputs
for the two limits. NVS keys `rclim`, `rdon`, `rdoff` persist the setting. Backup/
record schema 8 appends source/on/off fields; versions 1–7 remain readable and
explicitly disable prevention, even when the current configuration enables it.
Source/limit edits require the usual controller-runtime configuration apply.
Source availability checks prevent disabling/replacing the required paired
sensor while an enabled RainDetector references it.

Extended controller diagnostics add `rain_dew_config` (source, limits, maximum
age) and `rain_dew` (validity, demand, drying priority, dry state, gap when valid).
They use the existing source checkbox and UDP recorder. `actuator_output` remains
the authoritative software application timestamp. Existing rain events and
recognition parameters are retained. No frost override or analog/PWM power curve
is introduced by this step.

Validation (2026-11-11): 133 distinct native cases passed across RainDetector
policy (24), controller configuration (40), configuration backup/migration (20),
controller runtime (17), controller MQTT (19), property resolver (7) and derived
measurements (6). New coverage includes binary hysteresis and boundary values,
missing/stale/nonfinite/incoherent climate pairs, factory resolver injection,
frequency failure safety, drying priority/hand-over, localized form parsing,
persisted settings, source dependency validation, runtime-apply classification,
and schema-7 JSON/record import with prevention explicitly disabled. Translation
audit found no new uncatalogued strings in this feature; pre-existing findings
outside this scope were left unchanged. ESP32 build, flash and physical dew-point
validation remain pending with the user.
