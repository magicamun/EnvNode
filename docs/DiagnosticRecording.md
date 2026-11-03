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
SD-driver changes, filenames, filesystem access or event-name registration.

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
NVS and backup schema 5. Schemas 1–4 remain readable, with all new choices off.
Old positional NVS snapshots and JSON backups are tested separately.

The checkbox is editable with configured SD access and an inserted card. When
unavailable it remains visible and retains the saved choice. Disabled entities
do not emit even if the preference is retained. Saved settings do not imply a
producer already implements diagnostic events: Raindetector/SCT013-specific
instrumentation is later work in the respective application branches.

The first concrete producer is `SystemDiagnostics.cpp`; it uses the public
recorder only and contains no SD/filesystem dependency.

The SD page separately enables **Extended system diagnostics**. This produces a
runtime-health event on activation/card return and then once per minute (heap,
largest free allocation). Its explicit test button submits
`recorder_test` through the same public recorder contract. The test queues one
event; it neither formats the card nor creates a separate disposable test file.

Availability means configuration, pin ownership, stable runtime, allocated RAM
and mechanical card detect allow submission. It does not certify a FAT mount or
successful durable write. Real storage errors remain visible as write/retry
status. Card removal stops new submissions; previously accepted buffered records
may be retried on return. Settings changes discard stale queued generations and
clear pending diagnostic batches; an in-flight write may finish.

## Buffered persistence

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
