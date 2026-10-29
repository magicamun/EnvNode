# Test board firmware

EnvNode-owned materials follow the repository LICENSE
(LicenseRef-EnvNode-Use-and-Redistribution-1.0).

## Build targets

| PlatformIO environment | Application | Version source | Output relative to firmware/ |
| --- | --- | --- | --- |
| `esp32dev` | Existing EnvNode | `include/FirmwareVersion.h` (0.8.2) | `.pio/build/esp32dev/firmware.bin` |
| `testboard` | Fixture base | `src/testboard/TestBoardFirmwareVersion.h` (0.7.0) | `.pio/build/testboard/firmware.bin` |

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

Available pages: `/`, `/test`, `/hardware` (Provisioning), `/network`, `/time`, `/device`, `/logs`, `/firmware`.
The start page identifies the fixture and its current scope. `/test` provides a
guided simulated connector test with a revision-specific Weather or Mini definition,
KiCad board view, LEDs and contact-versus-socket matrix. Physical measurement and
tester configuration backup/restore are future work.
Measurement multiplexers/ADC and DUT power switching remain inactive. Provisioning
uses real I2C only during explicit page reads or confirmed identity writes.
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
provisioning bus is GPIO16 SDA / GPIO5 SCL; it starts only for each EEPROM operation.
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
composition and shared libraries. Forty-five tests exercise configuration isolation,
reboot/reset, setup-mode pages and route inventory, persistence, locale, explicit
restart, OTA staging and the guided simulation, including all 3268 comparisons,
open/extra connections, incomplete coverage, free connector selection, result history
and isolated retesting. The existing EnvNode native environment excludes this
fixture suite. Its previously known board-profile-count assertion is unrelated.

Before accepting the baseline on hardware, flash `testboard` onto the fixture
ESP or a spare devkit, check setup access point/network save/restart, locale/time,
name, logs and a same-product OTA update. Confirm that the start log identifies
Testboard 0.7.0. No physical device was flashed or restarted during implementation.

## Mini (0.3.1)

The catalog also exposes Mini 0.8 / PCB 0.8.1 (eight connectors, 1976 comparisons).
`GET /test/catalog` lists profiles; definition and SVG accept the `board` query.
Each connector requires explicit confirmation after moving the pigtail. There is no
automatic whole-board run, including in simulation. The former `all` action is rejected.

## Split fixture routing (0.5.0)

Revision-0.1 Controller/Input/Module PCB routing is generated by
`firmware/scripts/generate_fixture_routing.py` (`--check` detects stale outputs).
The simulator resolves logical contacts through this mapping without GPIO access.
Profiles explicitly select 38 or 40 inputs; input 39 is MB_3V3_SYS and input 40
MB_GND. Existing bundled profiles retain 38 inputs. Native tests verify every
mux channel, the GPIO assignment, reference comparisons and legacy compatibility.
Provisioning reuse is implemented in 0.5.0. Reference-aware Mini 0.9/Weather
0.3 profiles and remote profile delivery with a shared presentation model follow.

## External board provisioning (0.5.0)

`/hardware` is linked as **Provisioning**. It uses the same
`HardwareProvisioningWebRoutes`, `BoardProvisioningService`, `BoardIdentityStore`
and `IdentityEeprom24LC32` as EnvNode. The application adapter supplies manufacturing
profile metadata without importing EnvNode sensor/runtime resource selection.
Since 0.7.0 supported identities include Mini 0.8/0.9, Weather 0.2/0.3 and
Mainboard 0.3. Board and actual revision are selected explicitly; existing
numeric-ID API calls retain their old revision. Module provisioning uses the same
shared descriptor writer as EnvNode, independently of runtime-driver support.

The dedicated connector is GND / 3V3_SYS / SDA16 / SCL5; target EEPROM address is
0x50; module slot A/B EEPROMs use 0x52/0x53. Provisioning is real, even though `/test` remains a simulation. Remove the
target ESP32 and disconnect other supplies, Input Adapter and measurement pigtails
before connecting this cable. Disconnect it again before passive continuity tests. Its supply is wired;
ending I2C does not turn target power off.

Boot does not start I2C. Opening `/hardware` reads the current target once; a
confirmed write uses the existing invalidation/write/CRC/readback procedure for
the ENID identity record. Board provisioning now also writes a self-contained
ENPM port map in two banks at 0x0100/0x0800 before committing ENID. Bytes outside
ENID and these banks are preserved. The target ESP32 can then reconstruct its
resources without a downloaded profile or built-in revision table.
Module writes use the ENHD A/B-bank procedure and only the selected module address. Every operation ends I2C and restores
SDA/SCL to inputs, including failures. The bus has a 50 ms transaction timeout.
Missing/unprovisioned/invalid targets show their actual status; a failed refresh
clears any previous target identity. Writes still require server-side confirmation.
There is no tester restart after a successful external write.

While a connector test is running, GET `/hardware`, board-write and module-write aliases
return HTTP 409 before I2C access. The service also rejects direct read/write
attempts in that phase. There is no persistent provisioning session: synchronous
HTTP operations release the bus before the next test action can execute.
Eight native integration tests use the real EEPROM transport against a Wire double
and cover bus release, missing devices, readback failures, preserved bytes,
confirmation, aliases, hotplug state and mutual exclusion. Physical target
provisioning still needs validation on assembled hardware.

## Revision profiles and remote delivery (0.6.0)

Mini 0.9 / PCB 0.9.0 and Weather 0.3 / PCB 0.3.0 add explicit protected reference
paths and 40 measurement inputs. Their nominal simulated plans contain 2080 and
3440 comparisons. Existing 38-input profiles remain available unchanged.
The tester can obtain a bounded, validated catalog and compact profiles over
verified HTTPS from docs.envnode.de. Matching built-in profiles provide fallback;
the browser obtains the SVG directly. Publication uses main's regular Docs workflow.
See [remote profile format, operation and trust model](TestBoardRemoteProfiles.md).
The independent signature check and physical validation remain future work.
Revision-specific provisioning is included in 0.7.0 / EnvNode 0.8.2 below.

## Revision-specific provisioning (0.7.0)

Both applications share `BoardIdentityProfiles`; EnvNode 0.8.2 accepts the new
identities and selects the exact recorded runtime revision. Existing identities,
configuration and numeric-ID APIs retain their previous behavior. The tester adds
a revision-specific board image using the same definition/image routes as testing.
An unknown external EEPROM requires an explicit board/revision selection.
See [BoardRevisionCompatibility.md](BoardRevisionCompatibility.md).
