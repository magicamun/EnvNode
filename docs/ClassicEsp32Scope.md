# Classic ESP32 firmware boundary

EnvNode: see repository LICENSE.

Decision recorded on 2026-11-03: continue the current classic ESP32 architecture
on PlatformIO Espressif32 7.0.1 / Arduino-ESP32 2.0.17. Pause the separate
`chore/platformio-8-port` migration. Resume the new framework together with the
next hardware architecture, rather than maintaining a temporary Arduino 3
baseline on the current 4 MiB boards.

The classic ESP32 scope includes existing functionality, reliable SD operation
and integration of the RainDetector acquisition and heating controller.
SX1262 support is explicitly reserved for the next architecture and will not be
added to the classic ESP32 firmware. AnalogHydroPressure remains a candidate;
its inclusion depends on measured flash and runtime internal-RAM budgets after
RainDetector integration. No support commitment is implied by remaining flash
alone.

Keep the existing dual-OTA partition layout, configuration compatibility and
measurement continuity. Validate combined firmware size, startup, largest free
internal-RAM block, SD logging/web access and RainDetector control on hardware.
Build/test success alone does not establish runtime acceptance.

Implementation is isolated on `codex/esp32-rain-integration`, starting from
main `4b46d62`. Framework correction and RainDetector integration are separate
commits. Existing feature worktrees and uncommitted user work remain intact.
The SD lifecycle redesign (automount and explicit eject) is a separate functional
change; this backport retains the current per-operation mounts.

## Reproducible SD framework backport

Arduino 2.0.17 `sdcard_uninit()` frees its internal card structure while an
`AcquireSPI` guard still refers to it. The guard destructor then reads
`card->spi` to end the transaction. A local host regression using the actual
installed function reproduces this heap-use-after-free under AddressSanitizer.
The correction ends the guard scope before unregistering/freeing the card,
matching the upstream lifetime correction:

- [Arduino 2.0.17 source](https://github.com/espressif/arduino-esp32/blob/2.0.17/libraries/SD/src/sd_diskio.cpp)
- [Corrected Arduino 3.3.12 source](https://github.com/espressif/arduino-esp32/blob/3.3.12/libraries/SD/src/sd_diskio.cpp#L689-L706)

`firmware/scripts/prepare_sd_framework.py` runs before each Arduino build. It
checks Arduino 2.0.17 and the exact function SHA-256, copies the SD library into
the target's build directory and applies the correction there. The local copy
retains upstream sources, headers, library metadata and license notices.
Its library search directory precedes framework libraries, so SD.h and direct
sd_diskio.h access share the same corrected implementation. The global
PlatformIO package is never modified. Deleted build directories, fresh
checkouts and package reinstallations regenerate the correction automatically.
Unknown source/version changes stop the build and require explicit review.

After installing the Arduino package, run:

```sh
cd firmware
python3 scripts/tests/test_sd_framework_patch.py -v
pio run -e esp32dev -e testboard
```

The regression requires clang++ with AddressSanitizer. Set
`ENVNODE_ARDUINO_FRAMEWORK` to test a package outside the default PlatformIO
installation. Tests check the actual upstream and corrected functions,
reapplication, rejection of unrecognized source, unchanged upstream files and
stable generated sources. Native domain tests do not compile the SD hardware
driver and cannot replace this regression or a hardware test.

Validation: four SD patch regressions passed, including the AddressSanitizer
comparison. Both Arduino 2 firmware targets build successfully. EnvNode uses
1,615,389 of 1,835,008 program bytes and 105,012 static RAM bytes. The compiler
dependency file confirms that sd_diskio.cpp comes from the local corrected
library. Combined RainDetector validation and hardware acceptance remain open.


## RainDetector integration — 2026-11-04

The RainDetector branch is merged with the current main functionality, including
SD recording, hardware provisioning, derived humidity values and solar position.
The production composition supplies SensorManager as the local frequency source
and uses the locale-aware property adapters for event times and numeric display.
RainDetector description changes refresh retained MQTT metadata.

Published main IDs are preserved: SolarPosition remains implementation 9;
RainDetector is added as 10. Existing measurement IDs 1–16 remain unchanged;
frequency is 17. Existing unit IDs 13/14 retain density/angle; Hz/kHz use 15/16.
The combined backup/snapshot schema is version 6, appending frequency preference
after the version-5 fields. Stable main schemas 1–5 remain readable and default
frequency presentation to Hz, preserving their SD and diagnostic choices.
Experimental RainDetector branch builds had colliding IDs and a different draft
v4 layout; those draft encodings are not the stable main compatibility contract.
Review/recreate experimental configurations before using them on this baseline.

The controller's larger RainDetector history is owned only by instantiated
RainDetector controllers, rather than inflating all 16 slots of both runtime
banks. Frequency acquisition and policy service allocate no per-window memory.
Task-time checked allocation and the existing runtime failure handling remain.

Combined validation: 653 unique native tests pass, including a corrected rerun
of the 20-test configuration/backup suite; all 71 tester-native tests pass.
All three browser suites pass. Both classic Arduino 2.0.17 firmware targets
build with the project-local SD correction. Before the final commit identity
refresh, EnvNode uses 1,646,389 program bytes (89.7% of 1,835,008) and 101,492
static RAM bytes; tester uses 1,473,153 program bytes and 71,872 static RAM bytes.
These are build measurements, not a measurement of runtime free heap.

No firmware was uploaded and main was not updated. On-device RainDetector/SD
concurrency, startup and heating behavior, card removal/reinsertion, log download
and largest-free-block measurements are the next acceptance step. The patch fixes
the proven unmount use-after-free; it does not by itself resolve all earlier
low-memory mount failures or implement automount/eject.


## Weatherstation SD hardware checkpoint — 2026-11-04

The user reports that the integration firmware has been flashed onto the
existing Weatherstation (192.168.20.63), and the SD module moved from EnvNode
Mini is recognized and written successfully. This is user-observed hardware
validation of detection and writing with the classic Arduino 2.0.17 backport.
The supplied development image was built from integration commit c0daeea.

Reading/downloading the resulting files, longer-duration stability, heap/largest
free block and card removal/reinsertion are not yet confirmed for this run.
The firmware still uses per-operation mounts; automount/eject and RainController
specific diagnostic producers remain outstanding functional work.
