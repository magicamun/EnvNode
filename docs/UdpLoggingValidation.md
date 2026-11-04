# UDP logging validation ledger

EnvNode-owned: see ../LICENSE.

## A: SD baseline, 2026-11-04

Source HEAD f3ca1eee6bde530bb363a2dd70062cf5f29e02f4. Firmware source unchanged;
pre-existing deleted firmware/firmware.bin preserved. Build identity marks the
worktree dirty for that deletion. No upload or remote server operation.

Command: `~/.platformio/penv/bin/pio run -e esp32dev` in firmware.

- Platform espressif32 7.0.1; Arduino package 3.20017.241212+sha.dcc1105b.
- Xtensa toolchain 8.4.0+2021r2-patch5; release, existing flags/partitions.
- Program: 1,656,157 / 1,835,008 bytes; OTA reserve 178,851 bytes.
- Static RAM: 101,764 / 327,680 bytes (PlatformIO report).
- Runtime heap: not measured; hardware access not established.
- Local reference artifacts: firmware/.pio/logging-validation/baseline/ (ignored).
- Binary SHA-256: c98b1a3ae388decb6031a2c8d44448ab124620a4274854c54ebc13fc57b0c9f1.
- ELF SHA-256: 3bacccca1dc6ee23a08ea437fcdb65bcec7c3bbceae3fcbababa69475d9787d2.

The default build exports firmware/bin/envnode-0.8.7.bin (ignored); it does not
restore firmware/firmware.bin. The ELF and envnode-0.8.7.map are preserved for section/symbol comparison.

## B: UDP integrated, SD code retained

Build `0.8.7+468.g659fd15.dirty`, default receiver port **514**. Receiver address
remains empty/disabled until configured; the user's intended test destination is
`iot-rsyslogd` / `192.168.20.10:514/UDP`. Source dirty marker remains due to the
pre-existing deleted firmware/firmware.bin.

| Resource | A: SD baseline | B: UDP, SD code retained | B − A |
| --- | ---: | ---: | ---: |
| Program (PlatformIO definition) | 1,656,157 | 1,674,529 | +18,372 |
| OTA reserve | 178,851 | 160,479 | −18,372 |
| Static RAM | 101,764 | 101,980 | +216 |
| .dram0.data | 26,276 | 26,276 | 0 |
| .dram0.bss | 75,488 | 75,704 | +216 |

Program values exclude .flash.appdesc, matching PlatformIO's size definition.
Xtensa sizeof probe: LogEntry 280 bytes, DiagnosticRecord 608 bytes,
UdpLogBuffer object 180 bytes. Queues occupy 2,240 + 4,864 bytes, with one
2,048-byte codec buffer: 9,152 bytes of array storage and 9,332 bytes including
the buffer object, plus allocator overhead for four allocations. No UDP worker
stack is allocated. Actual ESP32 runtime heap/minimum/largest block and latency
remain to be measured. Do not interpret static RAM delta as total RAM cost.

Both esp32dev and testboard build successfully on the pinned framework.
Testboard reference: program 1,476,545 bytes, static RAM 72,064 bytes.
The testboard image was not uploaded.

Validation:

- Final single PlatformIO native run: **116/116 passed**, nine suites:
  configuration_backup, controller_runtime, diagnostic_recorder, display_service,
  firmware_runtime, structured_logging, web_host, rain_detector_policy,
  udp_logging.
- Six new UDP tests include every one of 14,400 rain windows through a real
  127.0.0.1 UDP socket, maximum escaped fields, metadata/time, sequence gaps,
  full buffers and fair service, offline/backpressure/error discard, recovery,
  and an accelerated one-hour gate stream with controller bursts/system logs.
  These are simulated time, not an hour of physical ESP32 runtime.
- `python3 tools/validate_udp_log_format.py`: three actual codec fixtures parsed
  as JSON, no duplicate keys, no field truncation. Normal rain data preserved.
- Existing RainDetector tests verify rejected recording has no effect on
  measurement/controller/heater behavior. Existing WebHost backpressure tests pass.
- An earlier accidental pair of overlapping PlatformIO test invocations collided
  in their build directory. Those errors were discarded; both processes were
  stopped and the final 116-case run was serial and successful.
- rsyslog/logrotate syntax, routing and rotation on an actual receiver remain
  pending. No server was changed. See tools/rsyslog/README.md for the examples
  and official documentation references.

Hardware-test candidate files (ignored, Finder-visible):
`firmware/bin/udp-validation/udp-with-sd/` contains app, bootloader, partition
binary, ELF, map, build identity and section report. This is a test candidate,
not a published release. A subsequent app-only upload must use the **actual**
partition layout and OTA selection, not blindly overwrite the partition table.

- App SHA-256: f3cdbdf9a615c2e74b5c89d4574cbf2a75f6fbf2fdbf45a66c684dc9d3221f8b.
- ELF SHA-256: c1e7018beb3dc514fa5813a52ee3f33fe608e757829778e0c1136d5aa900d757.

Hardware target identified by user: Weather mainboard 0.2,
`/dev/cu.usbserial-0001`. Initial passive serial capture contained no bytes.
Reading the partition table with esptool was blocked by a busy serial port
(Python monitor PID 51731). No flash/erase occurred. Await monitor release before
retrying. SD removal (C) remains after full-rate validation of the UDP path.

## Physical B checkpoint, 2026-11-04

The user authorized terminating the verified PlatformIO monitor PID 51731.
It was stopped; the monitor command was `platformio run --target upload --target
monitor`. Subsequent esptool access identified ESP32-D0WD-V3 revision 3.1, MAC
30:76:f5:e5:c6:14. Runtime board identity is Weather 0.2, HTTP address
192.168.20.63, and the previous installed build was
`0.8.7+464.gf3ca1ee.dirty`.

**Actual installed partition layout differs from the current build CSV:**
NVS 0x9000/0x5000, otadata 0xe000/0x2000, app0 0x10000/0x1c0000,
app1 0x1d0000/0x1c0000, SPIFFS 0x390000/0x70000. There is no cfg_nvs partition.
The partition table was preserved, not replaced. NVS/OTA bytes and the entire
previous active app1 were backed up locally under
firmware/.pio/logging-validation/baseline/. Configuration-containing files have
mode 0600 and are not committed. OTA entries had valid CRCs, VALID states and
sequences 5/6; sequence 6 selects app1. Prior app backup SHA-256:
272ba8a57f6226126be0467b8d948fe6fc29c3f4fda03db0f16a5a1f18d297ff.

A 460800-baud read failed with a serial packet error before any flash write.
The full backup and subsequent app-only upload succeeded at 115200 baud.
Only the candidate application was written at **0x1d0000**, with esptool's data
hash verified. Bootloader, partition table, otadata, NVS and SPIFFS were not
written. Candidate build `0.8.7+468.g659fd15.dirty` boots, connects WiFi/MQTT and
retains the device/sensor/controller configuration. BME280 I2C1/0x76 failure and
the controller's unusable-rain-input status existed before the upload; they are
not introduced by UDP logging.

The sensor-4 RainDetector checkbox was already enabled and remained enabled.
Controller-1 diagnostics were explicitly enabled for this test by posting the
otherwise unchanged edit form; firmware confirmed changes active without a
controller rebuild. Network logging was explicitly enabled with system logs and
system diagnostics to the user-designated **192.168.20.10:514**. A recorder_test
was accepted. A single existing read-only I2C scan produced four ordinary system
log entries, all four accepted by the local UDP stack, for receiver routing tests.
No rsyslog server settings were changed.

Twelve status/controller/measurement page rounds and a five-second nonreading
TCP-client probe completed. Diagnosis admission and local sends continued:
501 → 613 diagnostic records over the observed web-load rounds, then 855 on a
later sample, with no full/busy/offline/would_block/send_error/encode_error losses.
Queue high water was 6/8; observed queue count returned to zero. HTTP page
requests in the recorded rounds were approximately 0.13–0.34 s. The probe does
not prove that the TCP receive window actually saturated; do not claim complete
backpressure coverage from this physical probe alone. Native pump tests do cover
explicit backpressure. No actuator on/off/level commands were issued.

Runtime internal 8-bit heap samples (bytes) are checkpoint observations, not a
normalized memory-saving claim:

- Previous SD build, failed-automount log: free 45,596, largest 42,996.
- B, logging disabled and before load: free 57,972, minimum 32,156, largest 55,284.
- B, UDP first enabled: free 48,428, largest 42,996.
- B, after repeated HTTP/TCP probe: free around 40,840, minimum **10,484**,
  largest 28,660. After 120 seconds without HTTP requests, free heap recovered
  to 43,776; minimum stayed 10,484 and largest stayed 28,660. This does not
  establish a leak or a stable long-term memory budget.
- Maximum UDP service observed: **12,927 µs**, including configuration/socket
  work. Maximum application-loop body observed: **454,544 µs**, including
  unrelated web/network/startup work. This is not a complete worst-case bound.

Ordinary ESP.getFreeHeap status showed 79,808 bytes on A and 104,560 on early B;
those generic values must not be substituted for capability-specific 8-bit heap.
A does not have a measured lifetime heap minimum or identical HTTP-load trace,
so no comparable A/B runtime minimum delta can be claimed yet.

An optional redirect to a local capture receiver at 192.168.10.20:5514 was
**rejected by automatic approval review** because that extra destination/payload
had not been explicitly authorized. The redirect did not run; firmware remained
on 192.168.20.10:514. The unused local listener was stopped. Actual receipt,
full-rate gate continuity and separated receiver storage still require the user's
rsyslog observations or explicit authorization of that additional local test.
Sender sent_local counters alone do not prove receipt. SD removal remains gated
on that full-rate validation checkpoint.


Final B observation after the idle recovery interval: 1,651 diagnostic records
and four system records accepted/sent_local, queues empty, all local loss/error
counters zero, diagnostic high water 6/8 and system high water 4/8. The configured
destination remained 192.168.20.10:514. Full receiver continuity is still pending.
The new receiver analysis helper passed three Python tests covering wrap,
reordering, duplicates, gaps, separate boots and distinction between transport
sequence gaps and sensor-revision gaps.

## Current partition table installed on explicit user request

On 2026-11-04 the user explicitly requested the newer/larger partition table.
The current firmware/partitions.csv was compiled with the installed framework's
gen_esp32part.py and matched the already-built candidate partitions.bin exactly
(SHA-256 000720dbb6a6beb1f081495f1555461eb76b2074cdce9c3496e52e819d9ace58).
The difference is the dedicated 256-KiB cfg_nvs partition, not larger OTA slots:
app0/app1 remain 0x1c0000 bytes each, with unchanged offsets. The candidate
application already fit the previous OTA slots.

Before installation, fresh backups captured 0x8000–0xffff (partition table,
current NVS/OTA) and the entire old 0x390000–0x3fffff SPIFFS region under
firmware/.pio/logging-validation/partition-migration/, with restrictive umask.
Backup SHA-256 values:

- before-table-nvs-ota.bin: b29b24c3e3c27d7ab1a719d27d5917564c405e91c91ffe47c06e3485db49c7ff.
- before-spiffs-tail.bin: 6c28586364462df0ca5704154d2962e6d2a714a2443388b2a97220269188dcac.

The new cfg_nvs range was already entirely 0xff. No data-region erase was needed.
Only the partition-table sector at **0x8000** was written, at 115200 baud, with
esptool hash verification. The table plus NVS/OTA were then read back. Table
bytes match the current CSV-derived image and OTA metadata remained byte-identical.
Legacy NVS acquired two WIFI_STA_DEF system entries during the normal reboot;
it is therefore not byte-identical, and no such claim is made. The saved logger,
RainDetector sensor and controller edit forms compare unchanged before/after.

The device boots the same UDP application, reconnects WiFi/MQTT, retains six
active sensors and one running controller, and retains receiver 192.168.20.10:514.
As designed, table-only installation leaves legacy configuration authoritative;
the first verified snapshot restore switches authority to cfg_nvs. No restore,
factory reset, controller rebuild, bootloader upload or application rewrite was
performed as part of this partition-table update.

After restart the system-log queue reported 19 full drops while replaying startup
history into its bounded eight-entry queue. This is visible expected best-effort
startup overflow, not an unreported loss. A later observation showed 15 system
records and 204 diagnostics locally sent, with no diagnostic queue or send errors.
This does not replace receiver-side confirmation of full-rate continuity.

## Authorized local receiver attempt after partition installation

The user explicitly authorized routing system logs and diagnostics to their Mac
192.168.10.20:5514 for at most three minutes, then restoring 192.168.20.10:514.
The local listener ran for 180 seconds; the firmware was redirected for roughly
160 seconds, with a finally-block restoring the intended server. A separate
readback of the saved web form confirmed **192.168.20.10:514** afterwards.

No accepted ESP32 packets reached the local capture (0 diagnostics / 0 system
records). The user confirmed this path is blocked by a firewall. Route inspection
showed the Mac on 192.168.10.20/en5 through gateway 192.168.10.253 and the ESP32
on 192.168.20.63. The macOS application firewall reported disabled; this does not
rule out network ACLs or other filters. Packet capture was unavailable because
sudo required a password. No firewall, routing or server settings were changed.
Do not interpret this attempt as a successful end-to-end receipt validation.

During repeated HTTP requests, a four-second nonreading client and this blocked
UDP-destination interval, diagnostics continued locally: 896 → 1,459 accepted/
sent_local in the recorded 150-second sample span, with diagnostic full/busy/
offline/would_block/send_error/encode_error all zero. Controller/system context
is included in that count, so it is not a direct rain-window count. Maximum
observed logger service remained 14,402 µs, application body 452,049 µs. The
minimum internal 8-bit heap under this load reached **8,904 bytes**; later samples
were around 35 KiB free. The narrow memory margin remains a measured limitation,
not evidence that all network stalls or future sensor budgets are resolved.

The receiver analyzer now returns failure for an empty capture, preventing zero
received records from being mistaken for a passed test. Four Python tests pass.
Actual rsyslog receipt/full-rate continuity is still pending read-only server
access or receiver-side evidence. SD removal is still held at that validation
boundary.

## Receiver-side full-rate validation passed

The user provided root SSH access with an existing key. Connection by numeric IP
initially failed host-key verification; the already trusted hostname
`iot-rsyslogd` was used successfully, without disabling host-key checking.
Read-only inspection confirmed rsyslog 8.2302.0 active on 0.0.0.0:514/UDP and
actual ESP32 records in /var/log/remote/192.168.20.63/syslog.log.

A protected local copy contained **4,410 valid JSON records**, 4,361 diagnoses
and 49 system logs across four intentional test boots. Diagnoses included
4,255 rain_window, periodic controller context/decisions/outputs, health records
and recorder_test. No malformed JSON was found. The final 120-second interval
had 450 diagnoses, including 440 rain windows, **zero transport sequence gaps,
zero duplicate packets and zero sensor revision gaps/resets**. Every completed
measurement window was retained; no downsampling or averaging was introduced.

Measured gates in that interval ranged 250,038–372,730 µs with observed window
rate 3.659/s and maximum adjacent end-time delta 383,538 µs. All 440 windows had
fault=none. The nominal acquisition constant remains 250,000 µs; real main-loop
latency makes some gates longer. This proves complete transfer of actual windows,
not an exact 4-Hz hardware deadline or elimination of existing application stalls.
This receiver checkpoint satisfies the staged validation gate before SD removal.

The server currently writes both streams into a single per-source log. The
repository example for separate files passes `rsyslogd -N1` on that server.
The rotation example was adapted to root:adm, matching the inspected daemon,
and parsed successfully with logrotate --debug through a RAM-only memfd.
A first /dev/stdin attempt was ignored by logrotate and was not counted as a
successful parse. No production config, listener, rotation, firewall or log file
was changed; no service was reloaded. The prepared separate-storage/rotation
example has not been deployed.

## C: SD removed, final build and physical verification

Source commit **e39d774**, build **0.8.7+474.ge39d774.dirty**. The dirty marker is
still the pre-existing local firmware/firmware.bin deletion, excluded from every
commit. SD runtime, recording/file/web/formatting code, SD implementation tests,
activation/configuration/pin claims and the project-local framework patch are
removed. The generic descriptor catalog remains usable for EEPROM provisioning,
but compatibility reports MissingDriver and runtime inventory UnsupportedDriver.
Physical unsupported-module pins remain reserved by generic discovery.

Diagnostic producer events, checkboxes and per-entity NVS keys remain unchanged.
Backup schema 7 retains reserved legacy SD fields for positional compatibility;
exports always encode enabled=false and slot=0. Only old system/diagnostic choices
migrate; no setting can activate an SD driver. No framework migration was made.

Final tests and builds:

- **610/610 classic native cases passed**, all 58 selected classic suites.
- **71/71 tester-native cases passed**, all five tester suites.
- All three browser suites passed (17 profile cases, shared provisioning dialog,
  English/German page-editor checks).
- Codec JSON parser fixtures passed; all four receiver-analyzer Python tests pass.
- esp32dev and testboard release builds passed with Arduino 2.0.17 / platform 7.0.1.
- Defined-symbol inspection of the final ELF retains no EnvNode SD classes,
  SDFS/SDClass, SD disk I/O, SD card init/mount/uninit, or FAT mount/open functions.
  General FS/SPI support required by unrelated libraries is not claimed removed.

| Resource, bytes | A: original SD | B: UDP with SD code | C: UDP without SD | C − A |
| --- | ---: | ---: | ---: | ---: |
| Program, PlatformIO definition | 1,656,157 | 1,674,529 | **1,569,321** | **−86,836** |
| OTA slot reserve | 178,851 | 160,479 | **265,687** | +86,836 |
| Static DRAM | 101,764 | 101,980 | **100,776** | **−988** |
| .dram0.data | 26,276 | 26,276 | 26,096 | −180 |
| .dram0.bss | 75,488 | 75,704 | 74,680 | −808 |
| .iram0.text | 86,815 | 86,775 | 85,135 | −1,680 |

The final SD-removal step C−B frees 105,208 program bytes and 1,204 static DRAM
bytes. All use identical toolchain, optimization/profile and OTA slot sizes;
program reporting excludes .flash.appdesc consistently. C tester build is
1,476,529 program bytes / 72,064 static DRAM bytes. Array/object UDP heap budget
remains 9,332 bytes plus allocator overhead, with no UDP worker stack.

A fresh table/NVS/OTA backup and form snapshots were taken before the final flash.
The installed new table and OTA sequence/state still selected app1. Only the
final app at 0x1d0000 was written, with esptool data-hash verification. No server,
partition, NVS, OTA selection or bootloader rewrite accompanied that app update.
All three logger/Rain-sensor/controller edit forms compare unchanged after reboot.
WiFi/MQTT reconnect, six sensors and one running controller remain. Old /sd-card,
/sd-card/format and /sd-card/download now return 404; navigation has no SD link.
The physically inserted SD descriptor is displayed with MissingDriver.

Runtime C internal 8-bit heap observations:

- Before the final repeated-load probe: free 46,596, minimum 26,400,
  largest 24,564 bytes.
- Same 12-round status/controller/measurement HTTP probe and five-second slow
  reader used earlier: free around 38,252, minimum **14,900**, largest 24,564.
- Later observation: free 41,168; minimum unchanged. Maximum logger service
  **8,964 µs**, maximum application body **453,596 µs**.

For comparison, A's recorded failed-mount checkpoint was 45,596 free / 42,996
largest, with no comparable lifetime minimum. B's new-table early observation
was 43,364 free / 23,540 largest / 26,328 minimum; its later local-receiver HTTP
trial reached an 8,904-byte minimum. The earlier 12-round B probe, before cfg_nvs
installation, reached 10,484. These identify memory pressure and scenario conditions;
unequal uptime, network/allocator state and partition-cache state prevent a
controlled numerical runtime-minimum saving claim. Static and runtime figures
must not be combined into a guaranteed Analog/SCT013 budget.

Receiver verification after the C HTTP probe found **1,424 diagnoses**, including
**1,389 rain_window**, plus 16 system logs for the final build/boot. Diagnostics
have **zero sequence gaps/duplicates** and rain has **zero revision gaps/resets**;
all 1,389 windows have fault=none. Observed window rate was 3.659/s, gate minimum
250,040 µs, largest 527,018 µs including initial startup, and largest adjacent end
delta 390,603 µs. The nominal 250,000-µs acquisition constant is unchanged; every
actual completed window is sent, without traffic-driven averaging/downsampling.
This is a multi-minute physical checkpoint, not a long-duration field qualification.

The system stream has **18 sequence gaps**, matching the local **full=18** count
while bounded startup history is replayed into eight slots. These visible startup
losses are accepted best-effort behavior, not claimed zero loss. Diagnostic full,
busy, offline, would_block, send_error and encode_error stayed zero. A local
socket acceptance counter still cannot prove future delivery; this checkpoint
uses actual receiver-file evidence.

Final local hardware-test artifacts:

- firmware/bin/udp-validation/udp-without-sd/: app, bootloader, partition table,
  boot_app0, ELF, map, symbol/section report, manifest, README, license and hashes.
- firmware/bin/udp-validation/envnode-0.8.7-build474-no-sd.zip: flash/test package,
  without debug ELF/map or any configuration backup/captured user logs.
- App SHA-256: d17481160ec313dfd0e3659e54219b2152d9bdcf7fdf61e6e076dddd63f16367.
- ELF SHA-256: 4f375b38c2e32e65f53bb17b14512f3c6bb79e2faaa29b0fe6f72fbc1e775389.
- ZIP SHA-256: 5a0d04180fab4350c3022ee738ce057e8831b572a5d3c1c9477f240353aa360c.

The final device remains configured to **192.168.20.10:514/UDP**. The user-authorized
read-only SSH checks did not deploy the prepared split-file rules or rotation;
the live server still has its original combined per-source log. The syntax-checked
examples under tools/rsyslog are ready for a separately coordinated deployment.
