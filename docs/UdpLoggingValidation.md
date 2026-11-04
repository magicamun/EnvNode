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
