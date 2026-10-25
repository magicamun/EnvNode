# Shared firmware basis: incremental extraction

EnvNode-owned documentation follows LICENSE
(LicenseRef-EnvNode-Use-and-Redistribution-1.0).

## Scope

The working EnvNode 0.7.0 application is the starting point. Both the future
tester application and EnvNode will use local shared libraries in the same
repository. This step extracts logging only; it does not create a tester
application or change sensor, actuator, network, provisioning or storage APIs.

`firmware/lib/EnvNodeLogging` owns ten public headers and six implementation
files, moved byte-for-byte. Existing include names, namespaces, message format,
buffer sizes and serial behavior are unchanged. The native application source
filter no longer enumerates these six implementation files: PlatformIO compiles
them as the same local library used by the ESP32 build.

`SystemLogTimeProvider` remains an application adapter for system time and
`IMonotonicClock`; `LogWebView` remains in the current web application. Neither
is pulled into the logging library. Main application composition is unchanged.

## Validation on 2026-10-25

Baseline: firmware sources at commit `6567ee2`, FirmwareVersion 0.7.0.
Builds/tests ran on separate local snapshots to avoid NAS build-I/O overhead,
with the installed PlatformIO toolchain and matching dependency caches. Local
snapshots have no Git metadata; their generated build identities are not release
identities and their binaries are not intended as release artifacts.

Commands for both baseline and extracted version:

```sh
pio run -e esp32dev
pio test -e native
```

- ESP32 application build succeeds before and after extraction.
- Native test results match exactly by test file, test name and result:
  407 passed and one failed assertion before and after extraction.
- All 12 cases in `test_structured_logging` pass after extraction.
- All 16 moved source/header files match the baseline bytes.
- No firmware upload or physical smoke test was performed.

The pre-existing failure is
`test_on_off_hardware_validation::test_current_board_profile_describes_envnode_mainboard`
at `test_main.cpp:149`: `boardProfileCount()` is expected to return 2 but returns
3. The current registry contains EnvNode Mainboard, Weather and Mini profiles.
The same test also assumes index 2 is absent. This is separate from logging;
neither the registry nor the test was changed in this refactoring commit.

## Next boundaries

Extract common network/base configuration incrementally, preserving saved
configuration compatibility. Then split WebService into a shared server/page
frame, base administration pages and application-specific pages. Decouple
restart/OTA lifecycle from sensor runtime. Keep device identity and provisioning
of an attached target distinct. Add the tester application only after EnvNode
continues to build and passes its hardware smoke test on the shared basis.

Before merging the RainDetector and AnalogHydro branches, check file moves and
source filters explicitly; public logging includes and interfaces remain stable
in this step. The future sd-card work can use the extracted library directly.

## Step 2: network and base configuration

Two additional local libraries now define the next boundaries:

- `EnvNodeBaseConfiguration`: DeviceConfiguration, NetworkConfiguration,
  TimeConfiguration, LocaleConfiguration, the existing locale API and general
  validation rules extracted from ConfigurationService.
- `EnvNodeNetwork`: WiFiService and IWiFiService, with no application-level
  Configuration or sensor dependencies.

WiFiService accepts INetworkConfiguration, a read-only network view. Existing
IConfigurationService implementations inherit a concrete adapter returning their
existing Configuration.network reference. Existing construction sites and test
doubles continue to work; consumers must rebuild because the C++ constructor
parameter type has narrowed. Public include names remain available.

Validation bodies and limits were transferred without policy changes. NVS
namespaces/keys, defaults, complete Configuration layout, backup serialization and
transactional restore remain application-owned and unchanged. Persistent storage
is deliberately not split into a competing service in this step.

Validation on 2026-10-25, in a local source snapshot using the same cached
dependencies as step 1:

- ESP32 application build succeeds.
- Every pre-existing native test result matches step 1.
- All 13 new tests pass: eight WiFi lifecycle cases and five base-configuration
  cases, including legacy NVS reads, edits followed by reboot, and backup/restore.
- Overall: 420 passed and the same one pre-existing board-profile assertion
  failed. No new test failures.
- The host WiFi double verifies calls/state transitions, not radio behavior.
- No hardware was flashed; WiFi/AP/OTA smoke testing is still pending.

For sensor-branch merges, preserve the local library moves and remove obsolete
Locale.cpp application source-filter entries. Sensor and actuator APIs, NVS
formats and the current main.cpp composition were not changed. The next major
step is the web server/page-registration boundary; storage lifecycle and the
remaining services can then be disentangled incrementally.
