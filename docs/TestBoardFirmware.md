# Test board firmware

EnvNode-owned materials follow the repository LICENSE
(LicenseRef-EnvNode-Use-and-Redistribution-1.0).

## Build targets

| PlatformIO environment | Application | Version source | Output relative to firmware/ |
| --- | --- | --- | --- |
| `esp32dev` | Existing EnvNode | `include/FirmwareVersion.h` (0.8.0) | `.pio/build/esp32dev/firmware.bin` |
| `testboard` | Fixture base | `src/testboard/TestBoardFirmwareVersion.h` (0.2.1) | `.pio/build/testboard/firmware.bin` |

From `firmware/`, build explicitly:

```sh
pio run -e testboard
pio run -e esp32dev
```

`pio run` still builds EnvNode by default. In VS Code use the tasks under the
specific environment; choose `testboard` explicitly for fixture firmware. Both binaries
use the same existing ESP32/OTA partition table. A firmware upload or restart is
always a separate explicit operation.

For USB upload/monitor select the appropriate port under `[env:testboard]` in the
ignored `platformio.local.ini` (or pass `--upload-port` / `--port` on the CLI).
The existing local `[env:esp32dev]` connection settings are not inherited by the
fixture. The generated build metadata selects the version header per environment;
EnvNode and fixture versions can advance independently.

## Current scope

The fixture application is assembled in `firmware/src/testboard/`. It reuses
shared logging, base configuration/persistence, WiFi, time/locale, web page frame,
network/device/time/log/firmware routes, OTA/restart execution and runtime sequencing.
Only tester sources and shared libraries are selected for the fixture target.
EnvNode excludes the tester source directory. No EnvNode configuration model,
sensor/actuator/controller runtimes, MQTT, Home Assistant or display is linked
into the fixture binary. Shared libraries may compile unused reusable helpers;
linker garbage collection omits unused functions from the firmware.

Available pages: `/`, `/test`, `/network`, `/time`, `/device`, `/logs`, `/firmware`.
The start page identifies the fixture and its current scope. `/test` provides a
guided simulated connector test with a revision-specific Weather definition,
KiCad board view, LEDs and contact-versus-socket matrix. Physical measurement,
provisioning routes and tester configuration backup/restore are future work.
No multiplexer, ADC, DUT power or provisioning I2C pins are configured or driven.
See [TestBoardApplication.md](TestBoardApplication.md) for models, coverage and flow.

## Settings and first startup

Fixture settings use their own NVS namespaces: `testboard`, `testboard0`,
`testboard1`, `test_meta`. These are separate from EnvNode's `weather*` namespaces
and dedicated configuration snapshots. The baseline uses ordinary NVS key/value
settings; it does not implement a fixture snapshot codec yet. Factory reset only
clears the fixture namespaces. It does not erase the whole NVS partition or
EnvNode's configuration. Flash erase operations are outside this reset policy.

A fresh fixture does not read EnvNode WiFi credentials. It starts the shared
setup access point `EnvNode-Testboard-Setup`; connect and open
`http://192.168.4.1/` to configure the network. Saved WiFi changes and time-service
changes request a restart, with a visible explicit restart button. Locale and
device-name changes update the UI directly. Uploading firmware stages it and
keeps the restart explicit, as in EnvNode.

Hardware mapping remains fixed per application. The schematic's dedicated
provisioning bus is GPIO16 SDA / GPIO5 SCL; it is not started in this step.
The continuity engine and its safe enable/output sequencing must be implemented
and physically validated before testing boards.

## Validation and hardware follow-up

```sh
pio run -e esp32dev -e testboard
pio test -e testboard_native
pio test -e native
node test/browser/page_editor_test.cjs
```

The separate `testboard_native` environment links only fixture settings/web
composition and shared libraries. Twenty-two tests exercise configuration isolation,
reboot/reset, setup-mode pages and route inventory, persistence, locale, explicit
restart, OTA staging and the guided simulation, including all 3268 comparisons,
open/extra connections, incomplete coverage, free connector selection, result history
and isolated retesting. The existing EnvNode native environment excludes this
fixture suite. Its previously known board-profile-count assertion is unrelated.

Before accepting the baseline on hardware, flash `testboard` onto the fixture
ESP or a spare devkit, check setup access point/network save/restart, locale/time,
name, logs and a same-product OTA update. Confirm that the start log identifies
Testboard 0.3.1. No physical device was flashed or restarted during implementation.

## Mini (0.3.1)

The catalog also exposes Mini 0.8 / PCB 0.8.1 (eight connectors, 1976 comparisons).
`GET /test/catalog` lists profiles; definition and SVG accept the `board` query.
Each connector requires explicit confirmation after moving the pigtail. There is no
automatic whole-board run, including in simulation. The former `all` action is rejected.
