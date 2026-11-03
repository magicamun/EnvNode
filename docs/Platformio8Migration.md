# PlatformIO 8 / Arduino-ESP32 3 migration

EnvNode: see repository LICENSE.

## Scope and integration order

The `chore/platformio-8-port` branch starts from main commit `4b46d62`.
It upgrades the shared EnvNode/tester toolchain from PlatformIO Espressif32
7.0.1 / Arduino-ESP32 2.0.17 to Espressif32 8.0.0 / Arduino-ESP32 3.3.12.
Functional changes to SD mount ownership are deliberately a subsequent change.
Existing feature worktrees retain their previous toolchain until validation.

The platform version is pinned in `firmware/platformio.ini`. The GPIO PWM
adapter uses the Arduino 3 explicit-channel API (`ledcAttachChannel`,
`ledcWriteChannel`, `ledcDetach`) so runtime channel allocation remains owned by
EnvNode. Native Arduino stubs use the same API. The FatFs formatting adapter
uses `MKFS_PARM` with `FM_FAT32` and automatic geometry as before. The build
removes the framework default `CORE_DEBUG_LEVEL=0` before applying our level 3.
SD downloads use WebServer's `NetworkClient` explicitly. The tester HTTPS
profile transport uses the Mbed TLS 3 `mbedtls_sha256` entry point, preserving
error and digest checks. Startup diagnostic integer arguments match `%u`.
The effective Arduino 3 package flags did not include a compiler optimization
level. Explicit `-Os` is shared by both firmware environments to retain release
size optimization. The initial unoptimized images were 2,522,544 bytes (EnvNode)
and 1,927,068 bytes (tester), exceeding the existing 1,835,008-byte OTA slots.
The existing 4 MiB partition layout is preserved.

## SD framework correction

Arduino 2.0.17 `sdcard_uninit()` destroys its `AcquireSPI` guard after freeing
the card structure. The guard destructor reads `card->spi`, a use-after-free.
The reported SD writer crash enters `SPIClass::endTransaction()` through this
path. Arduino 3.3.12 contains an inner scope ending the SPI transaction before
freeing the card. No project framework patch is needed for that release.

Sources:

- https://github.com/platformio/platform-espressif32/releases/tag/v8.0.0
- https://github.com/espressif/arduino-esp32/blob/2.0.17/libraries/SD/src/sd_diskio.cpp
- https://github.com/espressif/arduino-esp32/blob/3.3.12/libraries/SD/src/sd_diskio.cpp#L689-L706
- https://docs.espressif.com/projects/arduino-esp32/en/latest/migration_guides/2.x_to_3.0.html

The standalone comparison sketch in `lab/SD-MountLifecycle` repeats 1000
mount/unmount cycles and offers Arduino 2 and Arduino 3 build environments.
It is a candidate reproducer; no hardware execution has been claimed.

## Acceptance before moving feature branches

Build both `esp32dev` and `testboard`, and run both native test environments.
Record resolved framework versions, firmware size and any compatibility changes.
A successful build does not establish runtime or hardware compatibility.

On the ESP32, verify boot and retained configuration, WiFi/MQTT reconnect,
web administration, OTA, I2C sensors, relay/PWM outputs and tester operations.
For SD, retain the existing per-batch mount/unmount behavior during migration:
verify recording, read/write diagnostics, directory listing and downloads,
card removal/reinsertion, and a run longer than the observed crash interval.
Formatting is destructive and requires a dedicated test card and explicit action.
Compare free internal heap, largest free block and SD worker stack reserve.

After hardware acceptance, integrate the migration into main and merge the
updated main into each active feature branch, resolving and checking each
branch's platform/API differences. Keep the original feature history; do not
rewrite shared branches. Implement persistent SD mounts and safe ejection only
on the new validated baseline.

## Validation status

2026-11-03: both firmware targets build successfully with PlatformIO 8.0.0,
Arduino package `4.30312.261024` (Arduino 3.3.12), ESP32 SDK libraries 3.3.12
and GCC 14.2.0. The installed SD source contains the corrected guard lifetime.

| Target | Static RAM | Program size | OTA slot |
| --- | ---: | ---: | ---: |
| EnvNode | 112,900 bytes | 1,775,844 bytes | 1,835,008 bytes |
| Tester | 75,208 bytes | 1,608,824 bytes | 1,835,008 bytes |

EnvNode has only 59,164 bytes of program-size headroom. Every feature branch
must therefore pass its own size check after integration. Static RAM is not
runtime free heap; SD mount heap/largest-block measurements remain a hardware
acceptance requirement. Remaining compiler warnings include existing aggregate
initialization, compact conditional statements and potential text truncation;
this migration does not claim a warning-free build.

The Arduino 3 standalone SD sketch builds successfully. All 627 main-native
and all 71 tester-native tests pass (698 tests total). `git diff --check` passes.
The firmware binaries are exported to `firmware/bin` for hardware acceptance;
they are development builds and do not constitute a published firmware release.

Hardware acceptance is pending. No firmware has been uploaded, no framework
installation was patched, and no existing feature branch has been migrated.
