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
placing the largest new implementation into every static DRAM slot. There is no
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
   before the first spray. The controller starts observation with heating off;
   it does not immediately heat. Manual Stop disarms it until Start or the next
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
wetting. Dry confirmation applies only inside a wetting-started 100% heating
cycle. An event cooldown of two seconds and heater guard of five seconds retain
the lab behavior. A 250-ms edge window can be too short at actual gate spacing;
the default is recommended for initial testing. These are experimental limits,
not calibrated specifications of rain intensity or surface moisture.

Optional temperature and humidity source selections are persisted and validated
against the selected sensors' capabilities. They are reserved inputs: no dew-point,
cold-weather or proportional preventive heating is active. They are not required
for the current wetting/drying policy. SD diagnostics remain deferred.

## Failure and stop behavior

- New wetting starts 100% heating and marks rain active.
- No newly detected wetting for the configured valid quiet time estimates rain end;
  heating remains independent until dry confirmation.
- Zero pulses, overflow, stale/missing source or invalid gates abort heating and
  reset rain to unknown. The started controller remains armed; after signal
  recovery and its guard, only new wetting can restart heating.
- Driver failure is retried; a temporarily unavailable target is resumed safely
  off when available. Manual output changes, including a partial PWM level while logically ON,
  disarm the controller and command the output off. Start is then
  required again. Stop commands output off and disarms the controller.
- Rain-end and dry are local estimates. Dry is a latched completion of the last
  heating cycle, not an ambient dry-surface measurement during cooling.

## Diagnostics and MQTT

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
in configuration backup/record version 4; versions 1-3 remain readable and use Hz.
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
