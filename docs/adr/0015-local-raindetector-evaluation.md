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
