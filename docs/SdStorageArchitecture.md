# SD diagnostic storage architecture

Historical backend reference. The classic ESP32 UDP candidate uses
[network logging](UdpLoggingImplementationPlan.md); SD runtime was removed after full-rate receiver
validation. Existing SD implementation remains available through Git.

EnvNode: see repository LICENSE.

Current public integration: **EnvNode 0.8.7**, merged into main on 2026-11-03.
See [the operating guide](site/firmware/sd-card.md) for current user steps.
Dated validation paragraphs below preserve the state at each checkpoint; pending
checks mentioned there are historical, with later results in
[SdCardSpiBringup.md](SdCardSpiBringup.md).

SD storage is bounded local diagnostic recording, not historical aggregation.
The normal firmware includes the driver. Without an identified module, manual
configuration defaults to disabled and selects breakout Slot A/B. A supported
module descriptor enables access and selects the slot automatically. System
logging and extended diagnostic preferences remain explicit persisted choices.
EEPROM-based SD module selection and the assembled module write/read test were
physically validated on 2026-11-03. Manual selection remains for the breakout.

Configuration is persisted in NVS and backup schema 5; schemas 1–4 remain
readable. Schemas 1–3 decode with SD disabled, and all older schemas default the
new extended-diagnostic choices to off. A runtime arbiter serializes access to the shared SPI SD device. The
writer uses an independent task, bounded RAM queue and bounded storage budget;
logger callbacks never perform SD I/O or wait for the card. Every batch closes
and releases the mount. Web operations acquire exclusive leases, retained for
an entire download or formatting operation. Opening a log for download/delete
seals its current part so subsequent recording writes a different part.

System logs are JSONL under /logs/system, with monotonic uptime and optional UTC
epoch time. Names use a persisted boot/session counter and part counter.
A new file begins with firmware/build/board/session metadata. Exact managed
filenames alone participate in rotation: 1 MiB per file, 32 MiB total, oldest
names first. Files are exclusively created; an existing file is never overwritten.
Flush/fsync occurs at most every 5 seconds or when a bounded batch fills; abrupt
power loss can lose unsynchronized entries. Queue overflow is counted and later
reported as one loss record. Card failures are retried with backoff. Configuration
changes and formatting start a new part; credentials are never session metadata.

First integration order: configuration and normal-build SD access, explicit test,
then queue/writer/rotation with fake-storage tests, then physical validation.

Validation on 2026-10-26: normal esp32dev build succeeded; 81 affected native tests
passed (configuration backups/migrations, structured and web logs, firmware
runtime, SD diagnostics/web/transfer/format and new buffering/rotation tests).
After adding the pending-runtime-configuration guard, the 18 affected SD web,
writer-policy and runtime tests passed again. German/English page-editor browser
checks passed. Default-build SD integration and recording still await physical
validation on the Mini. No firmware upload or remote card operation was performed.

Hotplug follow-up on 2026-10-27: physical recording recovery, download rotation
and reset persistence were confirmed, but directory HTTP handling blocked. Listing
is now an asynchronous, exclusively leased task, with lease release before atomic
completion publication. No totalBytes/usedBytes/f_getfree scan is performed; free
space is an optional FSInfo estimate. See SdCardSpiBringup.md for reproduction
and the still-pending physical retest.

An explicitly started, isolated hardware rotation diagnostic now reuses the
production append/pruning backend with fixed 8 KiB/24 KiB test limits in a new
exclusive workspace. Test data is read back and cleaned up; production limits
and /logs/system are unchanged. Its physical result is pending.

HTTP latency investigation (2026-10-27): the shared WebHost now measures SDK
response writes separately from the full handleClient call. It captures the URI
before the SDK clears it, counts bytes actually handed to the socket and records
shared page-header rendering time. A slow-request warning includes elapsed_ms,
first_write_ms (request processing up to the first send), send_ms and header_ms.
The remaining pre-response time can include parsing, body rendering and route
work; these are not yet distinguished. No response indicates that no measured
SDK write occurred, not proof of a parser timeout. Chunked-transfer framing and
custom download socket writes are outside these byte/write counters.

This step preserves the SDK parser, response framing and timeout behavior. It
adds observation before choosing a transport or timeout change. Read-only
network samples returned a 14 KB log page in 0.25 s and a 5 KB status page in
1.09 s, which does not support page size as the sole cause of stalls.

## General diagnostic boundary

The storage-independent `IDiagnosticRecorder` accepts bounded typed events from
system/module, sensor, actuator and controller sources. Producers neither name
files nor access SD/SPI/FAT. The application maps persisted per-entity flags and
card availability to source masks. A nonwaiting bounded queue passes records to
the existing background worker; JSON encoding is off the acquisition path.
System log and extended diagnostics have separate directories, queues, batches,
active-part seals and 32 MiB budgets while sharing the arbiter, session counter,
append/pruning backend and retry policy. Thus a future Raindetector or SCT013
producer changes only its own instrumentation/composition, not the SD driver.

The first real producer is optional system health; an explicit web test submits
a generic recorder event. Detailed sensor/controller producers follow in their
respective branches. See [DiagnosticRecording.md](DiagnosticRecording.md) for the
public contract, limits, configuration/migration and physical acceptance steps.
