# SD-Card SPI bring-up — Slot A

EnvNode: see repository LICENSE.

## Scope

SD support is now included in the normal `esp32dev` / Default build. There is no
automatic startup write test. Open **SD-Karte**, enable SD access, select the
physically wired breakout slot (A for the tested Mini) and save. Enable system-log
recording separately when desired. These settings persist in NVS and configuration
backup schema 4; older schemas restore with SD disabled. The previous
`esp32dev_sd_diagnostics` environment remains only as a compatible build alias.
The 4 KiB read/write test is now an explicit POST button on the SD page.

It requires no module EEPROM. The board identity selects the board profile;
The selected slot supplies its CS and Card Detect GPIOs. Shared SPI wiring is currently
fixed to the EnvNode core mapping (SCK 18, MISO 19, MOSI 23).

## Wiring (power off while connecting)

| Adafruit 254 pin label | Mini module slot pin |
| --- | ---: |
| 5V | 4 |
| GND | 1 |
| CLK | 13 |
| DO / MISO | 12 |
| DI / MOSI | 11 |
| CS | 14 |
| CD | 9 |
| 3V | unconnected |

Connect a 10 kOhm resistor from CD to module slot pin 2 (+3V3_SYS). This matches
the carrier's R1. CD is expected HIGH with a card, LOW without; verify the
physical socket's polarity. An internal pull-up is also enabled for this test.
Use short jumper wires. The initial transfer clock is 1 MHz.

The tested setup must have no other SPI device on the shared bus. The probe
rejects GPIO conflicts with enabled configured sensors/actuators and composed
module actuators before touching its pins. An enabled SD configuration reserves SCK/MISO/MOSI, CS and CD against later
sensor/actuator GPIO assignments. A nonblocking arbiter serializes all SD operations.
The CD input/pull-up and inactive CS level remain configured until another owner
or reboot changes them. This is not a general SPI bus/resource manager.

## Build and flash

From `firmware/`:

```sh
pio run -e esp32dev
```

Use `.pio/build/esp32dev/firmware.bin` for the existing Web OTA workflow. Default
Build and Upload now include SD support. No EEPROM is required for the explicit
breakout slot selection. The firmware version remains 0.8.0; its build identity
distinguishes the development build.

## Test

Hardware read probe confirmed on 26 October 2026 with a SanDisk Ultra 8 GB:
card capacity 7,948,206,080 bytes. Initially a Raspberry Pi image exposed only
58,662,912 filesystem bytes. After reformatting as FAT32/MBR, the filesystem
reported 7,931,645,952 bytes with 1,171,456 bytes used and a readable root.
Two resets after correcting loose jumper/breadboard contacts were free of the
previous CRC warnings. Startup without a card was confirmed by the user.
Physical write/read verification passed: 4096 bytes synchronized, closed,
reopened, compared and the test file removed; normal network startup continued.

1. Prepare a test card with an existing FAT16/FAT32 filesystem. Insert it before
   restarting. The probe never formats the card. It creates one exclusive test
   file named `/envnode-sd-test-00.bin` (or the first unused suffix through 31),
   writes a deterministic 4096-byte pattern, synchronizes and closes it,
   reopens it read-only and checks every byte plus the exact file length.
   It then removes only that invocation's own file. Existing files are never
   overwritten or deleted. If all 32 names already exist, the test fails without
   modifying them. After interruption/power loss a test file can remain; it is
   skipped on the next boot. Cleanup failure is logged with its path.
2. Open the existing System Log, or the serial monitor at 115200 baud. Look for
   `SD diagnostic:` entries: board/slot/pins, CD state, card type, card capacity
   in bytes, filesystem/used capacity and readable root directory.
3. Expect `SD write/read: PASS; 4096 bytes verified; own test file removed`,
   followed by `SD diagnostic: PASS; read/write verified; filesystem unmounted`.
   The normal measurement/web/MQTT loop then continues.
4. Power down/remove the card and reboot: expect `CD=LOW` and a skipped test.
5. After re-inserting the card, reboot to repeat. Hot-plug detection/retry during
   operation is deliberately not implemented in this first step.

Capacity checks include 64-bit values. The library combines SDHC and SDXC in one
type code. The breakout itself has no separately queryable SPI identity. CID
manufacturer/product/serial/date fields are not exposed by this initial adapter.

`SPI initialization or FAT mount failed` does not distinguish an electrical
failure from an unsupported/damaged filesystem (e.g. an exFAT card). Check both;
no automatic repair/format is attempted. Card Detect only reports the mechanical
contact and does not prove a successful SPI response. Removal during the probe
can make it fail; no completed probe is claimed until read/write verification
and cleanup succeed.
The explicitly requested write/read test remains synchronous and may delay
application servicing during driver retries. Directory reads now run in a
separate task, without a full free-cluster scan.
Automatic recording performs its I/O in the independent writer task; downloads
stream cooperatively. No diagnostic runs automatically at startup.

## Memory budget

Linking the SD driver exceeded the static DRAM region by 184 bytes. The existing
64-entry recent-log buffer (about 18 KiB) is now allocated once on the heap
instead of embedded in a static object; retention size and ordering are unchanged.
If allocation fails, recent-log retention is disabled (capacity zero), while
serial/log sink delivery continues. There is no per-message allocation. This
applies to both standard and diagnostic firmware builds.

## Software verification

```sh
pio test -e native -f test_sd_card_diagnostic
pio run -e esp32dev
```

Native tests cover absent-card skipping, cleanup after initialization failure,
64-bit capacities, unreadable root and invalid capacity reports, exclusive test
file creation, partial I/O, corruption and failure cleanup. These tests do
not replace the physical SPI/CD/power test above.

References: [Adafruit wiring](https://learn.adafruit.com/adafruit-micro-sd-breakout-board-card-tutorial/arduino-wiring),
[EnvNode module interface](EnvNode_Module_Interface_Hardware_Specification.md).

## First SD web page

The navigation entry **SD card / SD-Karte** opens `/sd-card`. Once enabled, it
shows the selected slot, card type/capacity, filesystem usage, file sizes and
browsable directories. The settings form remains available while SD is disabled.
Recording status includes queue occupancy and cumulative queue overflow count.

Directory requests start a background task, which mounts the card, reads
metadata and a directory page, then unmounts it. The immediate HTTP response is
a progress page that polls for the result; other web pages remain serviced. Refresh retries detection/mounting after inserting/changing a card;
no continuous Card Detect polling or permanently mounted card is needed yet.
Directory requests never run the write test. File download and confirmed deletion
are described below; confirmed formatting is described below. GPIO conflicts are checked before access,
including the current configuration and runtime module actuators.

Pages contain up to 32 entries in filesystem order. HTML output is also bounded
to about 12 KiB; exceptionally long escaped names can reduce the entries on a page. Previous/Next links cover
offsets through 1024; each background enumeration has a 2-second read budget.
A partial page offers continuation at the count actually displayed. Only the
hard offset bound stops paging; read-time exhaustion is reported separately.
Changing directory contents between requests may change page order/offsets.
Absolute directory paths are limited to 240 bytes; dot traversal, empty path
segments, backslashes and control characters are rejected. Names and links are
HTML-escaped and query values are percent-encoded. All handles are closed before
unmounting, also on read failures/removal. A card removed during the scan is
reported as absent; an I/O failure produces an error rather than a stale list.

This first request-based probe remains synchronous: mounting and FAT free-space
queries can delay the main loop. The scan budget cannot interrupt a blocking
filesystem/driver call. This is a lab file browser, not yet the nonblocking
recording/download service needed for timing-sensitive field operation.

Hardware validation: after flashing, visit `/sd-card`; inspect root and a
subdirectory, return to parent, then test refresh without and with the card.
The freshly formatted card may contain macOS metadata directories. For paging,
use a test directory with more than 32 files prepared on a computer.

Native checks:

```sh
pio test -e native -f test_sd_card_web -f test_sd_card_diagnostic -f test_web_localization
```

## Repeated-mount correction

Hardware testing exposed `esp_vfs_fat_register failed 0x(101)` after the first
successful web listing. In ESP-IDF 4.4.7, FAT VFS stores the mount path in
`base_path[ESP_VFS_PATH_MAX]` using `strlcpy(..., sizeof(base_path)-1)`.
With the SDK limit of 15, only 13 path characters survive. The former 14-character
`/sd-diagnostic` path could register but failed the exact-path lookup during
unregister; the two FAT contexts could be exhausted. `ESP_ERR_NO_MEM` therefore
did not by itself establish a heap exhaustion or card/filesystem failure.

All mount and POSIX file/directory operations now use the central short `/sd`
path. A compile-time check against the installed SDK limit and a native regression
check prevent recurrence. A restart is required to clear contexts left by the
old binary. Re-test root refresh and subdirectory navigation repeatedly (at least
ten requests), then removal and reinsertion. Repeated refresh and directory navigation were confirmed working by the user after installing the correction.

Source: [ESP-IDF FAT VFS register/unregister implementation](https://github.com/espressif/esp-idf/blob/v4.4.7/components/fatfs/vfs/vfs_fat.c).

## File download and confirmed deletion

Regular files now have **Download** and **Delete / Löschen** links. Directory
rows only navigate; neither the UI nor backend offers recursive deletion.
`GET /sd-card/download?path=...` opens the file and captures its length, sends an
attachment response and retains the client/file handles for a cooperative job.
The normal firmware loop sends at most one 1024-byte data block per step via a
nonblocking socket call. Partial sends retain the unsent buffer; sensor, network,
time, MQTT, controller and display servicing continue between steps. The web
server does not start other HTTP operations until the job ends, preventing
concurrent SD use/format/delete/OTA requests. No full-file buffer is allocated.
The existing SPI/FAT read/mount calls can still block internally on driver retries;
this is not a hard real-time guarantee for a failing card.

Downloads are terminated on disconnect, read/send error, 10 seconds without
progress, or 10 minutes total elapsed time. Files are sent only up to their
captured opening length. A truncated/interrupted transfer closes the connection
before its declared Content-Length, rather than reporting a completed download.
Range/resume requests are not implemented. Handles and the mount are released
on completion and failure. Unicode filenames use an encoded attachment filename
and a sanitized ASCII fallback; header control characters cannot be injected.

`GET /sd-card/delete?path=...` is read-only and displays the exact path and byte
size. Its form uses POST with a 128-bit random one-use token valid for two minutes.
The token is bound to the file path and its recorded size, modification time and
media fingerprint. Returning to the file list cancels it; even an invalid POST
attempt consumes it. The POST reopens a regular file, checks the recorded stamp,
then rechecks metadata before closing and unlinking that exact path. A changed
file/card must be confirmed again. No client-supplied file size/stamp is trusted.
The media fingerprint hashes boot/partition metadata and capacity, not hardware
CID; bit-identical cloned cards cannot be distinguished by this guard. Active
log-writer coordination/snapshots are still required before recording is added.

For hardware validation, put an expendable test file on the SD card using a Mac.
Refresh the file list, download it and compare its bytes/hash with the original.
Open Delete and cancel first; the file must remain. Then reopen Delete, confirm,
and check that only that file disappeared. Also check an interrupted download,
absence/reinsertion of the card and a repeated/expired delete confirmation.
Download and confirmed single-file deletion were physically confirmed working on the Mini on 2026-10-26.

Additional tests:

```sh
pio test -e native -f test_sd_file_transfer -f test_sd_card_web -f test_sd_card_diagnostic -f test_firmware_runtime
```

## FAT timestamp comparison correction

The first hardware file download succeeded, but confirmed deletion failed with
status 6 despite sufficient heap. The FAT SDK's `fstat()` returns st_mtime=0,
whereas `stat(path)` returns the real timestamp. Capturing the initial stamp via
fstat and comparing it with a path stat rejected an unchanged file. Capture now
validates descriptor/path regular-file status and equal size, but takes the time
from the path stat consistently. The metadata protection is retained. A native
regression exercises the FAT zero-descriptor-time case through ticket creation,
confirmation and the final path comparison. Delete failures also report their
specific stage and errno. The user confirmed successful hardware deletion after this correction.

Source: [ESP-IDF FAT fstat/stat](https://github.com/espressif/esp-idf/blob/v4.4.7/components/fatfs/vfs/vfs_fat.c#L525).


## Explicit FAT32 formatting

The normal build now offers **Karte formatieren / Format card**. The first version
requires a readable FAT card with a nominal capacity of 4–32 GB (capacity guard:
3.5 GB through 32 GiB). Unformatted/unreadable media and larger cards must still
be prepared on a computer. This restriction is intentional for the first hardware
test, not a FAT32 technical size limit.

`GET /sd-card/format` only inspects the card and displays Slot A, physical byte
capacity, and an explicit warning that **all partitions and files are lost**.
The user must type `FORMATIEREN` and submit POST. The server requires a 128-bit
random, one-use confirmation token valid for two minutes, bound to capacity and
media fingerprint. Wrong text/token, expiry or cancellation consumes/clears the
ticket. Switching to listing, download or delete cancels a format confirmation.
The fingerprint has the same cloned-media limitation described above.

Only the confirmed POST starts a FreeRTOS worker on core 1, at the application
loop's priority. HTTP servicing (including OTA) and other SD operations wait until
completion; sensor/controller/MQTT servicing continues in the main loop. The worker
uses 12 KiB of stack and a 4 KiB formatting buffer within that stack. It allocates
its own raw driver, uses the returned physical drive number, checks the card
fingerprint again before the first write, and refuses a nonautomatic volume
mapping. It calls the installed FatFs `f_mkfs(..., FM_FAT32, ...)` explicitly;
normal mounts retain `format_if_empty=false`.

With this SDK's automatic drive mapping, formatting **recreates the entire card
as a single MBR/FAT32 partition**, including removal of old Raspberry Pi partitions.
It is quick formatting, not secure erasure. The SDK's volume label API is disabled,
so no `ENVNODE` label is assigned. The old mount/driver is released, then the card
is remounted and its FAT32 boot signature, filesystem sizes and empty root are
checked. PASS is logged only after that check. Driver/task failures release their
resources and report failure, never automatic retry. A failure after writes begin
can leave an unusable filesystem. Do not remove the card, interrupt power or
update firmware during formatting. This is not a hard real-time guarantee; the
worker still consumes CPU/SPI time and is not forcibly aborted mid-format.

The result is available from **Ergebnis anzeigen / Show result**, or `/sd-card`.
Expected log on success:

```
SD format requested: slot=A card_bytes=7948206080
SD format: PASS; FAT32 remounted and empty root verified
```

The user confirmed successful hardware formatting on the Mini with the SanDisk
8 GB card on 2026-10-26. For repeat validation, first open
format and cancel, verifying files remain. Reopen, type the confirmation and start.
Wait for PASS, open the result and refresh the directory; it should be empty. Then
restart and run the explicit 4096-byte write/read test; it should pass and remove its
own test file. Check normal measurements/MQTT/control alongside the operation.
The device has not been remotely formatted during implementation.

Native validation:

```sh
pio test -e native -f test_sd_card_format -f test_sd_file_transfer -f test_sd_card_web -f test_sd_card_diagnostic
```

References: [installed SDK FatFs formatter (R0.13c)](https://github.com/espressif/esp-idf/blob/v4.4.7/components/fatfs/src/ff.c#L5080),
[ESP-IDF automatic drive mapping](https://github.com/espressif/esp-idf/blob/v4.4.7/components/fatfs/diskio/diskio.c#L19).

Implementation validation on 2026-10-26: 35 native SD tests passed (including seven
new format policy/form tests), German/English page-editor browser checks passed,
and `esp32dev_sd_diagnostics` built successfully. The user subsequently confirmed
successful formatting on the physical Mini.


## Buffered system-log recording

See [SD storage architecture](SdStorageArchitecture.md). Select the correct slot,
enable SD access and **Systemlog auf SD aufzeichnen**, then save. After the first
batch (normally within five seconds), `/logs/system` appears. Each JSONL part
starts with firmware/build/commit/board/session metadata; entries carry sequence,
uptime in milliseconds, nullable UTC epoch seconds, severity and escaped message.
No sensor diagnostics are enabled by this system-log option.

Example path: `/logs/system/boot-0000000042-part-0001.jsonl`. The monotonic session
counter lives in the separate `sd_log_meta` NVS namespace, advances once per boot,
and is deliberately excluded from configuration restores. If that counter cannot
be persisted, recording is disabled instead of risking filename reuse. Names are
created exclusively. Part counters stop at 9999 until the next boot; existing
names are skipped within a bounded search and never overwritten.

The RAM queue holds 28 structured entries (7840 bytes on ESP32); the writer holds
up to 4096 serialized bytes and uses a 12 KiB task stack. Serialization/storage
runs on core 1 alongside the application task. Logger callbacks only copy into the
queue under a short critical section; they do not wait for SPI, FAT or the network.
Healthy recording writes and fsyncs after five seconds or a near-full batch.
Popping stops when the pending batch is full. Overflow is counted and later
reported by a single summarized log entry. On card/full/filesystem failures the
pending batch is retained and retried after 30 seconds. Failed/partial writes
start a new part on retry: the old file may have an incomplete last line and
replayed entries may duplicate original sequence numbers. This is best-effort
diagnostic logging, not transactional or exactly-once storage.

Rotation limits parts to 1 MiB including metadata and managed data to 32 MiB.
Only regular files matching `boot-` + ten digits + `-part-` + four digits +
`.jsonl` inside `/logs/system` are counted/removed. The current part is protected;
oldest names are removed first. Unknown files and other directories are untouched.
Directory scans stop after 128 entries and pruning after 64 passes. A full card
occupied by unrelated files results in an error, not deletion of those files.

Each batch mounts and closes/releases the card. Web listing, tests, file access
and format share the same exclusive arbiter. A short concurrent write may return
**SD busy; retry**. A download retains the lease until completion, so the queue may
overflow during a long transfer. Opening the active log for download or delete
preview seals that part; recording resumes in a new part afterward. Confirmed
formatting verifies an empty root before releasing its lease; when logging is
enabled, `/logs/system` is then created again. Slot/enable changes discard pending
RAM data and start a new part, without appending old batches to a different slot.
Recording is not a hard real-time guarantee and physical validation is pending.

First physical checks: enable Slot A and logging in the Default firmware; wait
for a JSONL file, download and inspect it, then reboot and check a different boot
name. Download the active file and ensure the next log entry appears in a new
part. Remove/reinsert the card and check measurement/control continuity, error
reporting/backoff and recovery. Rotation/full-card behavior is covered with fake
storage natively but still requires physical qualification before field use.

Pending runtime configuration changes pause SD access/recording until applied or
restarted, so a stored GPIO assignment cannot be mistaken for the still-running
sensor hardware. SDK messages printed directly to Serial are not mirrored by the
structured EnvNode log sink.


## Hotplug web correction — 2026-10-27

The user confirmed system-log recording, download-triggered part rotation,
automatic writing recovery after removal/reinsertion, and persisted settings/card
contents across reset. However, the SD directory web page stopped responding.
During that interval the controller reported stale measurements (up to 50 s) and
MQTT publishing retried, so application servicing had also been delayed.

The installed Arduino SD implementation calls `f_getfree` in both `totalBytes`
and `usedBytes`. After each remount this may scan the entire FAT. Directory HTTP
handling called those methods synchronously. All such calls have been removed:
filesystem data capacity is derived from validated FAT12/16/32 boot geometry.
Used/free values come only from an optional, signature/range-checked FAT32 FSInfo
hint and are marked **≈**. Missing/unknown hints display **Nicht verfügbar /
Unavailable** instead of triggering a scan or claiming zero usage. These values
are estimates, not an authoritative full free-space count.

SD listing now performs mount, metadata and directory I/O in a worker task. The
main HTTP handler sends 202 with a polling link and preserves the requested path
and offset. Polling only checks atomic completion; it does not touch SPI/FAT.
Other web routes, measurements and integrations continue to be serviced. The
worker releases its lease before publishing completion, even if the browser
leaves the progress page. Results are bounded to one snapshot. No unsafe task
termination is used during a driver retry; the progress page can stay visible
until that retry returns. A pending read longer than three seconds is logged
from the live web loop. Other explicit SD operations (manual test, file open/read,
delete/format preview) retain their existing driver retry behavior.

This correction addresses the concrete blocking path found in the listing; its
physical hotplug retest is still pending. Reproduce with Slot A and logging on:
open the SD directory, remove/reinsert the card, refresh, and navigate to the main
page/logs while the SD read is pending. Verify normal measurement cadence and MQTT,
continued log files, and no reset requirement. Do not format to test this fix.

Software verification for the hotplug correction: Default esp32dev build passed;
51 affected native tests passed, including five new FAT-geometry/FSInfo/UI tests.
The SD lease test also checks explicit release at worker completion, repeated
release and preservation of a subsequent owner's lock. Physical retest remains
pending. The existing card/files have not been modified during development.

On 2026-10-27 the user confirmed the hotplug web correction works. The remaining
"Anzeigegrenze erreicht" message came from the original 150 ms directory budget,
not a full card. The asynchronous listing now allows 2 seconds per read request;
a time-limited partial page offers Next at the number of entries actually shown.
A zero-entry timeout asks for refresh and cannot generate a zero-progress Next
loop. The 1024-offset limit remains a separate hard listing bound.


## Isolated physical rotation test

The SD page offers **Rotation und Speicherbudget testen**, with an explanation
page and explicit POST start. This is not a startup test. It uses the same
`SpiSdLogStorage` append/rotation/pruning implementation as normal recording,
with fixed internal test limits of 8 KiB per file and 24 KiB total. Production
limits remain 1 MiB / 32 MiB. It does not fill the card or change configuration.

A worker acquires the exclusive SD lease and creates the first absent
`/envnode-rotation-test-00` through `-31` directory using exclusive mkdir.
Preexisting directories are skipped. A media fingerprint is captured before
creation and required before every batch, verification and cleanup. Keep the
card inserted during the test; this fingerprint has the cloned-media limitation
documented above. No other SD operation overlaps, while HTTP/main runtime remain
serviced and normal log entries buffer in RAM (overflow counting still applies).

The test writes 18 deterministic 2048-byte JSONL batches, 36 KiB of payload plus
session headers. With the scaled limits this creates six parts, three batches per
part. The oldest three must disappear, leaving exactly parts 4–6 within budget.
It verifies their sizes, exact data bytes and EOF, and verifies an unmanaged
`keep.txt` sentinel was not removed by pruning. It then removes only the bounded
set of filenames this test could create, its own sentinel and its own directory.
There is no recursive deletion, and production `/logs/system` is untouched.

PASS requires file rotation, pruning, preserved sentinel, content verification
and successful cleanup. No cleanup retry happens automatically after reboot or
on a different card; if cleanup fails, the result reports the leftover test path.
A card/filesystem failure reports FAIL. The test can run with SD access enabled
and system-log recording disabled. Its result page polls atomically published
completion, and a summary appears in the normal system log:

```
SD rotation test: PASS rotation=1 pruning=1 preserved=1 content=1 cleanup=1 ...
```

This qualifies rotation/pruning on real SPI/FAT media with small limits. It does
not simulate a physically full card or replace prolonged testing at the full
production budget. The first physical execution is still pending.

Implementation verification: Default esp32dev builds passed; 56 affected native
tests passed, including four new rotation-test fixture/policy/result tests.
The four tests were repeated successfully after adding the repeat-test form.
No test was remotely started on the device; physical execution is pending.


### Rotation diagnostic mount correction — 2026-10-27

The first physical rotation diagnostic created keep.txt and two complete
6241-byte parts, then failed at a later FAT VFS registration with 0x101. The
cleanup remount failed too. Verification had not run, so its result flags were
still zero despite successful initial writes. The exact resource-pressure cause
is not proven; 0x101 can indicate unavailable FAT contexts or allocator memory.
The existing test directory remained readable afterward.

The isolated diagnostic now owns one mount for workspace creation, all eighteen
writes, inspection and cleanup. It reuses the same mounted append/pruning backend;
normal recording retains its existing per-batch mount behavior. The mount is
released once, after cleanup, including all error paths. Native lifecycle tests
cover success, initial mount failure, workspace failure and simultaneous write/
cleanup failure. A cleanup error no longer overwrites the primary failure stage.
Failures also log start/finish heap and largest-allocation values for further
hardware diagnosis. See the [SDK VFS registration implementation](https://github.com/espressif/esp-idf/blob/v4.4.7/components/fatfs/vfs/vfs_fat.c#L112).

Default esp32dev build and 24 affected native tests passed after this correction.
Physical retry passed all five checks on 2026-10-27. The existing /envnode-rotation-test-00 workspace is
preserved; the retry chooses the next unused directory instead of reusing it.

The physical result was initially visible only after a manual browser refresh.
The progress page now polls a small status response every two seconds, replaces
its result section on completion and retries temporary network errors without
navigating away from the visible page. Manual result navigation remains available.
This UI correction does not establish the cause of the observed 25-second stale
measurement warning during the test; that remains a separate investigation.

Refresh correction validation: 16 affected native tests passed; a JavaScript
execution check covered running status, transient fetch failure, completion and
stopping further polls. Default esp32dev firmware build passed. Physical browser
verification of the new polling behavior passed on 2026-10-27: the result appears
automatically without manual refresh.

A subsequent physical run passed rotation but showed a transient FAT VFS
registration error, a system-log retry and a 20-second measurement delay. These
are not considered resolved by the rotation PASS. Diagnostic output now labels
failed mount operations (including before/after heap and largest allocation),
reports rotation phase durations and reports WebServer calls exceeding one
second. This distinguishes background SD work from a blocking HTTP request
without silently retrying or changing mount ownership.

Diagnostic instrumentation validation: 16 affected native tests and the normal
esp32dev build passed. The next hardware run is needed to localize the remaining
mount failure and main-loop latency.

Checkpoint before WebServer latency investigation (2026-10-27): the latest
physical run passed all five rotation checks and automatic result display, with
no mount or recording error in that run. Rotation itself took 3707 ms (mount
54 ms, writes 2391 ms, verification 354 ms, cleanup 406 ms), whereas a WebServer
handleClient call took 7911 ms. The route was empty after request handling, so
this instrumentation does not yet identify the request or distinguish parsing,
rendering and sending. Blocking HTTP latency remains open; the earlier sporadic
FAT VFS registration failure is also not considered fixed by one clean run.

Web latency measurement follow-up: shared WebHost captures the response route
during the first SDK write, before WebServer clears its URI. Warnings now include
first_write_ms, send_ms, header_ms and actual written bytes. Large first_write_ms
with small send_ms points to work before sending (parsing or handler work); large
send_ms points to socket writes. header_ms isolates the common page header only,
not the route-specific body. A no-response label means no observed SDK write.
Normal page navigation is sufficient for the next physical observation; another
SD rotation run is optional. These counters measure latency, not CPU usage, and
do not themselves fix the blocking behavior.

Validation: 35 affected native tests passed (WebHost timing and framing,
firmware web, web logs, SD rotation); default esp32dev firmware build passed.
Physical evaluation of the detailed request timing is pending.
