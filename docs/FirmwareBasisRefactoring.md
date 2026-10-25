# Shared firmware basis: step 1

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
