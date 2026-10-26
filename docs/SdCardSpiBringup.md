# SD-Card SPI bring-up — Slot A

EnvNode: see repository LICENSE.

## Scope

The explicit `esp32dev_sd_diagnostics` build runs a one-shot SD probe at startup,
after board identity/configuration and application initialization, before the
normal application loop starts. Normal `esp32dev` builds do not activate it.
This is a lab diagnostic, not the future SD module/storage subsystem.

It requires no module EEPROM. The board identity selects the board profile;
Slot A supplies its CS and Card Detect GPIOs. Shared SPI wiring is currently
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
module actuators before touching its pins. It does not reserve pins against
later web configuration changes: all SD resources are released after the probe.
The CD input/pull-up and inactive CS level remain configured until another owner
or reboot changes them. This is not a general SPI bus/resource manager.

## Build and flash

From `firmware/`:

```sh
pio run -e esp32dev_sd_diagnostics
```

Use `.pio/build/esp32dev_sd_diagnostics/firmware.bin` for the existing Web OTA
workflow, then restart. USB upload is also possible with this environment and
an explicitly verified upload port. Do not use the normal `esp32dev` binary for
this test; it has no active SD probe. The firmware version remains 0.8.0; the
build identity distinguishes this development build.

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
Mounting/reading is synchronous at startup and can delay startup, especially
when counting free FAT clusters. It runs once, not in the acquisition loop.
Ongoing nonblocking recording and bounded download handling are later steps.

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

The navigation entry **SD card / SD-Karte** opens `/sd-card`. In the explicit
`esp32dev_sd_diagnostics` build it shows current card type/capacity, filesystem
usage, file sizes and browsable directories. Other builds report that access is
not enabled. The existing startup write/read test remains enabled in the lab build.

Each page request mounts the card, reads metadata and a directory page and then
unmounts it. Refresh retries detection/mounting after inserting/changing a card;
no continuous Card Detect polling or permanently mounted card is needed yet.
Directory requests never run the write test. File download and confirmed deletion
are described below; formatting is not implemented yet. GPIO conflicts are checked before access,
including the current configuration and runtime module actuators.

Pages contain up to 32 entries in filesystem order. HTML output is also bounded
to about 12 KiB; exceptionally long escaped names can reduce the entries on a page. Previous/Next links cover
offsets through 1024; enumeration has an additional 150 ms scan budget. A
visible limit message replaces further paging when those limits are reached.
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

The lab build now offers **Karte formatieren / Format card**. The first version
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
restart: the existing 4096-byte startup write/read test should pass and remove its
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
