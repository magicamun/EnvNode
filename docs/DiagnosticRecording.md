# Extended diagnostic recording

EnvNode-owned content: see repository LICENSE
(LicenseRef-EnvNode-Use-and-Redistribution-1.0).

## Producer contract

`firmware/lib/EnvNodeDiagnostics/include/IDiagnosticRecorder.h` is the public
boundary. It contains no Arduino, SPI, SD, file, directory or JSON dependency.
A producer receives an `IDiagnosticRecorder&`, a source kind and a stable source
ID. The firmware composition installs the buffered recorder; without a sink,
`diagnosticRecorder()` provides a no-op recorder. A stable forwarding facade also supports producers constructed before storage
initialization. A new producer/event needs no
transport changes, filenames, filesystem access or event-name registration.

Example for a future sensor implementation (not Raindetector instrumentation):

```cpp
#include "IDiagnosticRecorder.h"
using namespace EnvNode;

void recordRawSample(IDiagnosticRecorder& recorder, uint16_t sensorId,
                     double raw, double baseline, bool wet) {
    const DiagnosticSource source(DiagnosticSourceKind::Sensor, sensorId);
    if (!recorder.enabled(source)) return;
    DiagnosticEvent event("raw_sample");
    event.number("raw", raw);
    event.number("baseline", baseline);
    event.boolean("wet", wet);
    (void)recorder.record(source, event);
}
```

Source IDs for sensors, actuators and controllers are their configured slot IDs,
starting at 1. System uses ID 0. Module IDs are physical slot A=1 and B=2; module
recording shares the extended-system-diagnostics opt-in. The generic buffer
supports up to 32 IDs per non-system category; application settings select only
actual entity slots. Event names and field names belong to the producer.

Each event contains at most six named values: finite double, signed 64-bit
integer, bool or UTF-8 text (at most 32 bytes). Names are nonempty ASCII letters,
digits, dot, underscore or hyphen; event names allow 31 bytes and keys 23 bytes.
Duplicate keys, invalid UTF-8, oversized text, NaN/Infinity and invalid source IDs
are rejected. A failed field insertion invalidates the whole event rather than
silently writing partial data. Accepted bounded events encode within a 2048-byte
line buffer, including worst-case escaping. Integer digits are preserved;
consumers should use an integer-capable JSON parser for values above 2^53.

`record()` performs bounded RAM work and does not allocate, access a filesystem,
wait for storage or retry. It returns Accepted, Disabled, Unavailable, Invalid,
Full or Busy. Full/Busy are counted. Do not call from an ISR; submit from the
sensor/controller task. High-frequency producers must choose an appropriate
sample/window rate. Measurement and control must continue regardless of the
result. Avoid a system-log message on every rejected diagnostic event.

## Settings and availability

Sensor, actuator (including module-actuator) and controller forms offer
**Extended diagnostics**. The saved choice defaults to false and persists in
NVS and backup schema 7. Schemas 1–6 remain readable; schemas 1–4 default
the diagnostic choices to off.
Old positional NVS snapshots and JSON backups are tested separately.

The UDP candidate keeps selections editable whenever the logger buffer is
allocated, independently of receiver or WiFi availability. Disabled entities do
not emit; saved preferences survive outages. System diagnostics and system logs
are separate selections under `/network-logging`. A destination is not activated
implicitly when old SD preferences migrate. Schema-7 exports retain reserved
legacy SD fields for positional/backward compatibility, but always encode SD
enabled=false and slot=0; these fields cannot activate hardware. Older saved
system/diagnostic selections migrate to network logging. The sensor/controller flags retain
their NVS keys and meaning.

SystemDiagnostics produces runtime health on activation and once per minute.
The network logging test button submits `recorder_test`. Automatic events use
the public recorder; manual markers use the bounded explicit-admission path
described below. Producers perform no network I/O. Every completed RainDetector
gate is still submitted at nominal 250 ms, without averaging or downsampling.

## Classic ESP32 UDP recording — 2026-11-04

The classic ESP32 composition routes recording to UDP. After the receiver
continuity checkpoint, SD runtime, web management, formatting, tests and the
framework patch were removed. See
[implementation plan](UdpLoggingImplementationPlan.md),
[measured validation ledger](UdpLoggingValidation.md), and
[receiver example](../tools/rsyslog/README.md).

Queues hold eight system entries and eight diagnostic records. Encoding uses a
single 2-KiB buffer. Each application loop attempts at most one datagram per
stream with a nonblocking numeric-IPv4 socket; failures are counted and dropped.
There is no writer task, filesystem batch or persistent replay in this path.
The web status exposes full/busy/offline/backpressure/send/encode counters and
queue occupancy/high water. Receiver silence does not prove a send failed.

Every message carries a factory device ID, random per-boot ID and build identity,
with the original sequence and capture timestamps. Diagnostics add source-mask
selection_generation; their valid selected attempts consume a sequence before
potential queue rejection. Counters/sequences wrap at 32 bits. Existing event
names, typed fields and sensor revisions are unchanged. Complete extreme text
events can exceed the usual MTU and fragment; they are never silently truncated.

## Manual test markers

On **Network logging**, the **Test markers** form accepts a short test name and
optional note. The extended-diagnostics checkbox links directly to that form;
save the sensor/controller settings before following the link. Marker actions
never change diagnostic choices, sensor/controller configuration or NVS.

1. Enable and save the desired sensor/controller detail diagnostics.
2. Enter a test name such as `Head 0.4 trocken` and a note such as `Controller 0.3`.
3. Choose **Mark begin**, perform the test, optionally **Add note**, then **Mark end**.
4. A standalone note can announce the next test without opening an interval.

Each field is limited to **32 UTF-8 bytes**, matching the existing diagnostic
text budget. Oversized/invalid text is rejected, never truncated. Notes outside
an interval have `test_id: 0`; their entered name is retained in RAM for the next
begin action. Begin allocates a monotonically increasing nonzero test ID for
this boot. Begin/end/notes during an interval carry its ID and original label.
Only one interval is active. Text and session state are RAM-only; a reboot has
no active interval and starts a new ID space. Pair intervals using
**device_id + boot_id + values.test_id**, never test_id alone. An unfinished
interval is not automatically given an invented end when the device restarts.

The new event is in the existing diagnostic stream and sequence space, with
system source ID 0. The following is a payload excerpt; device/boot/build,
sequence, selection generation and capture timestamps are supplied by the
existing recorder/UDP envelope:

```json
{
  "type": "diagnostic",
  "source_kind": "system",
  "source_id": 0,
  "event": "test_marker",
  "values": {
    "phase": "begin",
    "test_id": 1,
    "label": "Head 0.4 trocken",
    "text": "Controller 0.3",
    "origin": "web"
  }
}
```

`phase` is `begin`, `note` or `end`. Existing measurement events are unchanged;
markers define time boundaries and are not copied into every rain record.
They do not reset or align measurement gates. Inspect markers in a JSONL export:

```sh
jq -c 'select(.event == "test_marker") | {device_id,boot_id,sequence,uptime_ms,values}' capture.jsonl
```

An explicit operator marker is independent of system/sensor/controller detail
checkboxes. The concrete recorder admits only `test_marker` through this path;
normal producer masks retain their existing behavior. Such queued markers
survive source-mask generation changes, so unchecking a sensor does not erase a
just-accepted end marker. They still share the same eight-entry queue and send
budget, without priority eviction, a second queue, extra text buffers or retries.
A session uses at most 48 bytes of fixed storage. Rain windows remain full rate.

The POST at `/network-logging/marker` validates the phase, UTF-8 text, expected
active test ID and originating boot ID. Old forms, duplicate begin/end actions
and conflicting tabs return 409 without another marker. Invalid input returns
400. Disabled logging, disconnected WiFi, unavailable/busy/full buffers return
503. Session state advances only after RAM admission, so rejected begin/end
operations can be retried. Success redirects (303) back to the marker form,
preventing a page refresh from resubmitting the POST. Repeated explicit note
submissions are separate point markers.

**Accepted means RAM admission, not receiver delivery.** UDP outages or a later
logging shutdown may still lose a queued marker; normal transport counters and
stream sequences expose those gaps. The UI says receipt is unconfirmed. Marker
rejection does not affect measurements, controller output or diagnostic choices.

Validation: ten new native marker cases cover pairing/IDs, stale contexts,
UTF-8/length limits, unchanged selection, full buffers and retries, mask changes,
offline transport, UI escaping/status and 14,400 full-rate rain records with
146 interleaved markers. Existing recorder, real UDP loopback, RainDetector and
shared web tests also pass (79 distinct native cases in total). Per the user's
workflow, no ESP32 firmware build, flash, device POST or rsyslog change was
performed for this feature. Firmware compilation and hardware receipt validation
remain with the user's next build/flash and test.

## Historical SD buffered persistence

The following describes the previous SD backend and its historical validation,
not the active recording target in the UDP candidate.

One worker services two independent streams under the shared SD arbiter:

- `/logs/system`: the existing system log;
- `/logs/diagnostics`: typed extended diagnostic JSONL.

Both use boot/session metadata, `boot-NNNNNNNNNN-part-NNNN.jsonl`, exclusive
creation, 1 MiB parts and a 32 MiB managed-file budget per directory (64 MiB
combined). Exact managed names participate in pruning; other names are not
removed. Unrelated card files can still exhaust physical space. Downloading an
active part seals that stream's part; later records start a new file.

There are eight fixed-size diagnostic queue entries and a 4096-byte batch,
independent of the system-log queue. Concurrent producers use a nonwaiting lock;
contention and overflow return immediately. The existing batch/retry policy is
shared: flush after five seconds or a full batch; storage failures retain the
batch and retry after 30 seconds. Busy storage retains it without error backoff.
File I/O, encoding, rotation and pruning happen in the background worker.

Events contain `schema_version: 1`, type, recorder sequence, source kind/ID,
event name, typed values, uptime and optional UTC epoch time (`null` before time
synchronization). Queue/contention/unavailable/stale losses are counted and
reported later as `diagnostic_loss` records when possible. Counters are also
visible on the SD page. Configuration changes may discard accepted pending
batches; power loss can lose buffered data. Partial-write retries may duplicate
events or leave a partial line. This is best-effort diagnostics, not a durable
transaction log. Credentials/configuration secrets are not automatic metadata.

## Acceptance

Native tests cover source gating/no-op behavior, overflow/order, time capture,
policy generations, invalid values/UTF-8, typed JSON and maximum escaping,
concurrent producers, shared retry batching, disabled-checkbox rendering,
NVS/reboot and schema 1–4 migration. Existing system-log and storage tests remain
applicable because the same backend and pruning/batch policies are used.

Hardware validation: enable extended system diagnostics on the SD page, submit
the test event, wait for the next successful batch write and inspect/download
`/logs/diagnostics`. Check `runtime_health` and `recorder_test`, UTC/uptime, and
that `/logs/system` remains separate. Validate card removal/return and retained
checkbox settings. Raindetector-specific records are not expected yet.

Software validation on 2026-11-03: 81 affected native tests passed (nine recorder/
producer/UI-policy tests, 16 backup/persistence/migration tests and the existing
system-log, web, SD transfer/format, actuator and runtime suites; overlapping
system-log cases counted once). The first default ESP32 build passed. The final
build includes the independent system producer and form card-change policy.
No diagnostic event was written remotely to the physical card by the agent.

Memory follow-up: FAT mounts reserve one regular-file slot; all operations are
serialized and use no more than one regular file at once. The shared worker uses
12 KiB, as before the diagnostic extension, with its stack minimum exposed as
writer_stack_min_bytes. Runtime-health memory fields explicitly measure internal
8-bit RAM; generic internal-heap figures can include other memory capabilities.
Two physically written diagnostic files have been parsed successfully as JSONL.
Intermittent web-mount registration failure remains under memory-budget validation.

Physical follow-up: user log for Build 430 / 3097aeb confirms both recording
streams resume automatically after a requested software restart and a 527-byte
diagnostic file downloads fully. No mount/write error occurs in that supplied
run. The shared worker retains over 6 KB of measured stack reserve with its
12 KiB stack. Per-entity flag persistence/migration remains covered by native
tests; source-specific Raindetector/SCT013 instrumentation is subsequent work.
HTTP send latency remains a separately open item.

## RainDetector producers — implementation contract, 2026-11-04

Enable Extended diagnostics on the RainDetector sensor to record every completed
local gate (nominally 250 ms), independently of the MQTT/publication interval.
`rain_window` uses the sensor slot ID and contains `revision`, `start_us`,
`end_us`, `pulses`, `frequency_hz` and `fault`. Invalid gates and acquisition
failures remain visible. Fault names use the acquisition interface; `none`
means valid. Non-finite frequency values are omitted, never written as NaN/Inf.
Recorder uptime is the submission time; start/end are acquisition monotonic times.

Enable Extended diagnostics separately on its RainDetector controller to record
`rain_decision` on changes of validity, wetting, rain state, heat request or dry
confirmation. Fields: `sensor_id`, `revision`, `valid`, `wetting`, `rain`,
`heating`. Revision refers to the sensor window consumed by the controller.
Unchanged evaluations are suppressed; this is not one decision record per gate.

`rain_output` records lifecycle, fault/recovery and output changes. Fields are
`sensor_id`, `running`, `dry`, `output_percent`, `reason`, `operation`.
Output percent is read back from the actuator interface, not inferred from the
heat request; it is omitted when the target is absent/uninitialized. Reasons
identify start, stop, manual override, target loss/recovery and input conditions.
Operation reports the existing controller return result. Repeated unchanged
stale input, unavailable target or failed writes do not flood the queue.

On diagnostic activation/card return and every 60 seconds, `rain_config` and
`rain_edge_config` repeat the effective policy and source/target IDs, followed
by current decision/output snapshots. Config fields are respectively
`sensor_id`, `target_actuator_id`, `dry_hz`, `hysteresis_hz`, `dry_seconds`,
`end_seconds`, and `sensor_id`, `edge_ms`, `drop_hz`, `drop_permille`. A zero
explicit actuator ID may indicate a module-bound target; keep the configuration
backup to resolve its module identity. Optional temperature/humidity references
are not policy inputs yet and are not recorded as decision inputs.

Producers depend only on IDiagnosticRecorder, check enabled() before constructing
an event, allocate no per-event heap and perform no file I/O or retries. The
existing six-field, eight-entry queue and SD worker budget are unchanged.
Rejected records never change measurement, heater commands or policy state.
After a full queue, deduplication still advances; missing transitions are covered
by loss counters and subsequent periodic snapshots, not guaranteed replay.

Gate recording is intended for field diagnosis and consumes storage continuously.
The existing 32 MiB diagnostic rotation budget remains in force; this is not a
promise of days/weeks of complete retention. Enable sensor and controller
recording together for raw-window/decision correlation, and download parts
before rotation removes them. No automatic mount/eject change is included.

## Persistent mount and safe eject — 2026-11-04

The shared SD worker mounts automatically after startup/card insertion (300 ms
insertion debounce). SPI and FAT remain mounted between recording batches and web
operations. All accesses retain the existing exclusive nonblocking storage lease;
normal clients only borrow the mount. A failed mount is cleaned up and retried
at most once per 30 seconds. No automatic formatting is performed.

The SD page's **Eject SD card** button stops admission of new system/diagnostic
records and new web accesses. The worker drains accepted records and batches,
including fsync/close, and waits for existing downloads/directory/format jobs to
release their lease before unmounting. Refresh the page and wait for **unmounted /
safe to remove** before removing the card. A write error retains the pending batch
and prevents a successful eject indication; it is not silently discarded. Eject
is a runtime state, not a saved configuration change. Reinsert the card to resume.
The 250-ms RainDetector recording rate is unchanged.

Unexpected card removal invalidates the mount after the current lease is released.
Queued data from that mount are discarded rather than replayed onto another card;
diagnostic queue discards count as stale. Reinsertion debounces and mounts afresh.
Formatting is an explicit exclusive maintenance operation: it detaches the shared
mount, uses the raw formatter, then creates a fresh shared mount for verification.

## Live source selection — 2026-11-04

Saving only a sensor, actuator or controller's extended-diagnostics checkbox takes
effect in the shared recorder without requesting a runtime rebuild. Saving an
otherwise unchanged slot also leaves the running composition alone. Effective
settings are compared explicitly by implementation, without comparing object
padding or allocating a serialized configuration. Simultaneous hardware,
schedule, identity, source, target or policy changes retain the existing apply
requirements; an already pending runtime action is not cancelled.

### Automount eligibility correction

Hardware eligibility is independent of mount state, runtime stability and lease
ownership. The lifecycle combines eligibility with runtime stability separately;
file clients additionally require a mounted card. This prevents a startup cycle
where the automount permission itself requires an existing mount. The eject button
remains visible whenever SD support is enabled, but is disabled until access is
available; the adjacent SD state explains whether mount is pending or failed.

Regression validation: 36 hardware/runtime-effect tests and 12 SD lifecycle/log
buffer tests pass, including hardware eligibility before any mount exists and
while the storage lease is occupied. On-device startup/listing/eject validation
remains required.
