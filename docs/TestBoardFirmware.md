# Test board firmware

EnvNode-owned materials follow the repository LICENSE
(LicenseRef-EnvNode-Use-and-Redistribution-1.0).

## Build targets

| PlatformIO environment | Application | Version source | Output relative to firmware/ |
| --- | --- | --- | --- |
| `esp32dev` | Existing EnvNode | `include/FirmwareVersion.h` (0.8.2) | `.pio/build/esp32dev/firmware.bin` |
| `testboard` | Fixture base | `src/testboard/TestBoardFirmwareVersion.h` (0.9.0) | `.pio/build/testboard/firmware.bin` |

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

Available pages: `/`, `/test`, `/selftest`, `/diagnostics`, `/hardware`
(Provisioning), `/network`, `/time`, `/device`, `/logs`, `/firmware`.
`/test` defaults to real LOW/HIGH measurement on Controller 0.1 + original
Input-Adapter 0.1. Simulation is an explicit alternative and cannot silently
replace a real measurement. Switching modes clears the entire runner session.
The revision-specific board view, LEDs and result matrix use the same runner.
Tester configuration backup/restore and DUT power switching are not implemented.
No measurement GPIO or I²C is initialized at boot; an explicit operation is needed.
See the [user manual](site/firmware/test-provisioning/index.md) and
[TestBoardApplication.md](TestBoardApplication.md).

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
The real continuity engine is integrated since 0.9.0. Controller/input-adapter
loopbacks are verified; full DUT/reference-diode acceptance and threshold tuning
remain part of hardware commissioning.

## Validation and hardware follow-up

```sh
pio run -e esp32dev -e testboard
pio test -e testboard_native
pio test -e native
node test/browser/page_editor_test.cjs
```

The separate `testboard_native` environment links only fixture settings/web
composition and shared libraries. Fifty-eight tests exercise configuration isolation,
reboot/reset, setup-mode pages and route inventory, persistence, locale, explicit
restart, OTA staging and the guided simulation, including all 3268 comparisons,
open/extra connections, incomplete coverage, free connector selection, result history
and isolated retesting. The existing EnvNode native environment excludes this
fixture suite. Its previously known board-profile-count assertion is unrelated.

Before accepting the baseline on hardware, flash `testboard` onto the fixture
ESP or a spare devkit, check setup access point/network save/restart, locale/time,
name, logs and a same-product OTA update. Confirm that the start log identifies
the selected Testboard version. Hardware flashing/restarts remain explicit user actions.

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
numeric-ID API calls retain their old revision. Module provisioning is not exposed yet.

The dedicated connector is GND / 3V3_SYS / SDA16 / SCL5; target EEPROM address is
0x50. Provisioning is real; `/test` now offers explicit real and simulated modes. Remove the
target ESP32 and disconnect other supplies, Input Adapter and measurement pigtails
before connecting this cable. Disconnect it again before passive continuity tests. Its supply is wired;
ending I2C does not turn target power off.

Boot does not start I2C. Opening `/hardware` reads the current target once; a
confirmed write uses the existing invalidation/write/CRC/readback procedure for
bytes 0–31. Other EEPROM bytes are untouched. Every operation ends I2C and restores
SDA/SCL to inputs, including failures. The bus has a 50 ms transaction timeout.
Missing/unprovisioned/invalid targets show their actual status; a failed refresh
clears any previous target identity. Writes still require server-side confirmation.
There is no tester restart after a successful external write.

While a connector test is running, GET `/hardware` and both board-write aliases
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

## Manual controller diagnostics (0.7.1)

Open `/diagnostics` or **Hardwarediagnose** on the tester. This mode drives real
GPIOs on Test-Controller revision 0.1. Since 0.9.0 the connector test can also
use physical I/O through the same shared driver.
No measurement GPIOs are initialized at boot. Explicitly confirm the controller
and apply the settings to activate the diagnostic mode. Provisioning and simulated
test actions are blocked while diagnostics are active; starting diagnostics during
a running simulated test is rejected.

With the controller unpowered, remove the DUT and provisioning cable. Supply the
multiplexers through 3V3_SYS and common ground. If ORing is not installed, a temporary
connection from the DevKit 3V3 pin to J8 pin 1 supplies this rail; remove that bridge
before installing ORing. Never connect 5 V to COM_IN / GPIO34.

Select U1 or U2, channel I0–I15, and LOW/HIGH on GPIO17. Input EN1–EN3 and channel
selection can also be exercised at J8; leave all inputs disabled without an adapter.
GPIO34 is displayed as a 12-bit raw ADC count, not calibrated volts. The stop button,
network loss, or 60 seconds after the last apply command disables all multiplexers
and returns GPIO17 to input mode. Status polling does not extend that interval.
Closing/navigating away from the page sends a best-effort stop; the timeout remains
the fallback. A controller reset restores the inactive boot state.

For an initial loopback, connect J1 pin 1 to J8 pin 9 while unpowered. Select U1/I8
with all input multiplexers disabled. Compare LOW and HIGH ADC readings. R6 (1 kΩ)
and R9 (2.2 kΩ) divide the HIGH voltage to approximately 2.27 V before multiplexer
resistance; a full-scale ADC count is not expected. This is a manual commissioning
tool, not an automatic continuity certification or a validated physical test result.

## Browser response delivery (0.7.2)

The tester uses the same bounded nonblocking HTTP response pump and idle-client
gate as main. Page delivery yields between writes, and browser preconnections
without request data cannot monopolize request parsing. Firmware diagnostics
continue between response steps. Remote HTTPS profile downloads remain synchronous
and may still briefly delay requests when refreshing or loading a remote profile.

## Guided physical fixture self-test (0.8.0)

`/selftest` guides manual loopback wiring for Controller 0.1 and the original
produced Input-Adapter 0.1. The new DevKit adapters are not supported by this test.
Select any row while idle. Confirm the displayed bridge, then start that one test;
there is no run-all function because each contact requires manual rewiring.

The first 22 rows test J1/J2/J3 output contacts directly against controller J8 pin 9
(COM_IN), without the input adapter. The next 40 rows use controller J3 pin 2 as
source and test J4 pins 1–38 plus TP1 pin 2 (MB_3V3_SYS) and TP1 pin 1 (MB_GND).
These last two are free adapter reference contacts, not a connection to an actual
mainboard. Do not attach a DUT or provisioned target during fixture self-test.

Each nonblocking run checks target LOW and HIGH, disabled-path isolation, and all
other mapped channels in the same group. The other unused physical mux channels
are not covered. After switching, allow 15 ms to settle and acquire 8 ADC samples
at least 2 ms apart. Preliminary commissioning limits: every LOW/isolation sample
at most 200, every HIGH sample between 1000 and 3800. The report contains mean LOW,
mean HIGH and maximum isolation reading. These are raw ADC values, not calibrated
continuity resistance or a production acceptance threshold. A bad source, missing
supply or wrong bridge can also produce failure; no exact fault diagnosis is inferred.

All paths are disabled on completion, cancellation, network loss or a ten-second
run timeout. Manual diagnosis, provisioning and simulated-test actions cannot
change the hardware during a guided run. Background profile loading is deferred
while diagnostics or an HTTP response is active. Results live in firmware RAM;
the browser additionally retains completed readings in sessionStorage for this tab,
so switching power off to rewire does not discard the collected report. Download
JSON before closing the tab. Saved readings reflect previous runs, not an ongoing
measurement. “Ergebnisse löschen” clears firmware results and the browser session.

## Physical connector measurement (0.9.0)

`Esp32FixtureIo` implements the electrical switching once for diagnostics,
self-test and `PhysicalFixture`. Hardware access lives outside the runner and DUT
profiles. `PhysicalFixture` resolves pins via `FixtureRouting`, drives LOW, waits
20 ms, samples eight times, drives HIGH, waits 40 ms and samples eight more times.
Samples are taken at least 2 ms apart; the cooperative read method never sleeps.
The driver never reads expected connection masks.

All LOW samples must be ≤200. All HIGH samples ≤200 mean isolation; all between
800 and 3800 mean continuity (including forward reference diodes). Intermediate,
noisy/boundary-crossing or saturated readings are measurement errors. A 500 ms
per-path timeout releases outputs; the outer runner also bounds busy responses.
Limits are preliminary commissioning constants and require real DUT validation.
R6 and R9 plus a diode, pull-ups and capacitors affect measured values.

`SelectableFixture` arbitrates simulation versus the physical backend. Device boot
uses physical mode, with no GPIO side effects until start. Real start requires an
explicit server-side confirmation. Mode changes clear all connector results and
are rejected during a run. The simulated-fault selector is hidden in physical mode.
The web state always includes the actual backend mode. Diagnostic/self-test and
provisioning remain mutually exclusive with a running connector test. Loss of
network administration availability aborts and releases the active runner.
