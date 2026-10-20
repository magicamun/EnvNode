<!-- EnvNode-owned documentation: see ../../LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0). -->
# ADR-0015: Local RainDetector Evaluation

## Status

Accepted scope; algorithm and parameter selection remain experimental.

## Date

2026-10-19

## Context

RainDetector evaluation and its results must be available directly in EnvNode.
Normal operation must not depend on exporting the high-rate frequency stream for
external analysis. Logs and the offline analysis tool support development and
validation. This requirement refines ADR-0001 for this sensor: RainDetector may
evaluate physical surface wetting and the resulting local transitions, including
configurable estimates of rain onset and rain end. The user's explicit decision
on 2026-10-19 authorizes this narrowly scoped extension.

The experiments show rapid frequency drops after new wetting, including on an
already wet head with unchanged 100% heater output. Short positive trends also
occur between successive spray pulses. Visibly dry and thermally stable are
different conditions. Contamination and temperature shift the baseline frequency.
Partial-duty PWM produces interfering plateaus. With limited sensitivity, the
absence of detected drops does not establish the absence of further water input.

## Decision

- Acquisition remains behind a sensor abstraction. Raw frequency, measurement
  validity and heater status are explicit inputs to evaluation.
- An independently testable, stateful evaluator processes measurements
  incrementally with bounded memory and no dynamic allocation in the measurement
  path. Normal operation requires neither extensive history nor external analysis.
- Filtering separates a fast path for new wetting from a slower path for recovery
  and drift. One long moving average would suppress short wetting edges. Filter
  time constants and detection rules must be compared against the recorded logs;
  this decision does not establish validated numerical thresholds.
- Wetting events use rapid relative changes and an absolute minimum change to
  reject counting noise. There is no fixed global dry frequency. Zero signal,
  overflow, data gaps and restart produce explicit invalid or unknown evaluation.
- A local rain-end estimate requires sustained recovery and a configurable
  confirmation interval without newly detected wetting. The required timescale
  is minutes. A completely dry surface is not a prerequisite. Absence of events
  alone is insufficient; startup on an already wet surface remains unknown.
  Detection sensitivity and confirmation time are configured separately.
- Heater control and wetting evaluation have separate responsibilities. Current
  experiments use full ON/OFF heating. Heater transitions and thermal settling
  must be considered by evaluation. Dew-point control needs its own suitable
  measurements and cannot be inferred from frequency alone.
- MQTT publishes current measurements, state estimates, quality information and
  events at suitable rates. Normal operation does not require publication of
  every 250-ms measurement window. Publication rates are a later configuration
  decision.
- Raw frequency and diagnostics remain accessible. High-rate raw logging is an
  optional laboratory and troubleshooting facility.
- These experiments do not derive rain intensity, precipitation amount, weather
  forecasts or statistics.

## Implementation order

1. Replay existing logs, including the prospective spray experiment, and expose
   filter latency and missed short events.
2. Test the shared evaluator with bounded state independently, then exercise it
   live in the lab project. Parameter changes against known data do not replace
   a new independent experiment.
3. Add sensor/evaluator integration, NVS configuration, web diagnostics and MQTT
   to the normal EnvNode firmware once evaluation is independently usable. Do not
   integrate several unfinished subsystems at the same time.

## Current limitations

The first shared C++ evaluator is in `firmware/lib/RainDetectorEvaluation` and runs
in the lab firmware. It uses running filter state instead of a ring buffer and
requires 112 bytes per instance in the tested ESP32 configuration. Fast and slow
frequency filters and a smoothed rate of change are exposed for diagnostics. The
current decision uses the fast frequency drop and smoothed rate of change; the
slow frequency filter is initially diagnostic.

The normal production firmware still has no RainDetector integration or automatic
heater control. The original offline tool retains its documented, frozen
one-second-median detector. Its missed short pulse at the end of the spray
sequence remains part of the validation record.

## Open experiment proposals (2026-10-20)

Discussion points recorded on 2026-10-20; these are proposals, not validated requirements.

- Define configurable wetting detection, end-of-wetting confirmation and a separate dry-state criterion, including meaningful parameter limits.
- Investigate frequency bands as qualitative surface-wetting categories. Rain intensity and percentage scales remain unvalidated; temperature, contamination and water distribution confound frequency.
- Investigate full heating after wetting to accelerate recovery. Define heater inputs separately, including suitable temperature/humidity measurements for dew-point control and possible surface-temperature feedback.
- Consider a linear heater-power ramp between temperature/dew-point-related limits; determine endpoints experimentally.
- First characterize the dry head at ascending and descending heater levels, initially in 10 percentage-point increments, with separate runs of 1–5 minutes per level. Partial PWM interference must be distinguished from thermal drift.


## Lab policy revision (2026-10-20)

The user accepted a simpler experimental rain-end policy: a rapid negative raw
frequency edge marks new wetting; 60 seconds of valid observation without another
edge estimates rain end (configurable 30–300 seconds). Sustained recovery is no
longer required for this new lab policy. Rain status, surface-dry assessment and
heater-cycle state remain separate. The original shared evaluation core and its
replay remain unchanged for comparison; `LabRainAutomation` implements the new
policy independently in the lab, not in production firmware.

An explicit `AUTO ON` arms observation with the heater off. New wetting starts
full heating. Only within that cycle, at constant 100% output, a provisional
32000-Hz threshold with 500-Hz hysteresis and ten-second confirmation ends heating.
The user accepts this installation-specific dry estimate pending field data.
No new wetting is inferred from a slow threshold crossing after cooling.
The edge detector experimentally permits partial PWM; automatic output is ON/OFF.
Heater transitions pause observation for five seconds. Invalid data discard rain
history and dry confirmation; they cannot establish rain end or dryness. A heating
cycle is aborted on invalid samples; AUTO remains armed. Signal return starts
a five-second guard, then a fresh wetting edge is required to restart heating.
This supersedes the earlier retain-heating-on-invalid lab policy, after its
behavior was exercised on hardware. Explicit OFF also disarms AUTO.
Configuration is volatile and console-only in this lab stage. Hardware validation
and later sensor/NVS/web/MQTT integration remain separate work.

## Production port boundaries (2026-10-20)

Port increment 1 extracts the hardware-free `RainDetectorPolicy` into
`firmware/lib/RainDetectorEvaluation`. The lab now calls that exact implementation;
its console parser remains in the lab. The legacy recovery evaluator is retained
for historical replay only. This increment does not register a production sensor
or controller yet.

The production acquisition sensor owns only its configurable frequency input and
publishes raw frequency plus explicit validity/diagnostics. A separate actuator
owns the configurable heater output and safe OFF lifecycle. A RainDetector
controller resolves sensor and actuator identities through existing runtime
interfaces, evaluates copied measurements with the shared policy and applies
heater intentions through actuator capabilities. No driver pointer belongs in
application, web or MQTT logic. High-rate acquisition/evaluation must continue
independently from MQTT publication rates; a slow latest-value cache alone must
not lose subsecond edges. Resource ownership, measurement freshness and runtime
replacement must be resolved before registering the complete controller.

Temperature and humidity sources are optional controller references, not sensor
GPIO configuration or acquisition dependencies. Missing climate sources do not
prevent wetting detection and dry heating. Dew-point prevention and a configurable
cold-temperature heating rule (discussion example: below 4 degrees C) are deferred;
no climate heating is implemented by this port increment. Future independent
heating reasons must prevent dry-cycle completion from cancelling required cold
heating. SD logging remains deferred until the SD hardware is available. Normal
operation never depends on a card.
