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
