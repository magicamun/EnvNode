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

## Runtime confirmation after step 2

The user flashed the refactored firmware and confirmed that the existing
configuration, including sensors, was present and working. This confirms the
reported basic runtime/configuration check; it does not establish a complete
OTA, setup-AP, radio or actuator validation matrix.

## Step 3: shared web host and page frame

`EnvNodeWeb` now owns the HTTP server, shared stylesheet, HTML escaping and
page-frame rendering. Applications implement IWebRoutes and register handlers
with the host. EnvNode WebService is the current application provider and keeps
its existing page handlers, navigation and localization. A native fixture route
provider demonstrates use without constructing SensorManager or RuntimeManager.

The same 61 URL/method registrations are retained, including uploads and aliases.
The stylesheet is transferred byte-for-byte and keeps its cache header. Existing
page handlers still use the same server instance owned by WebHost. The header,
content and footer continue to stream separately; application fragments render
sequentially. NVS/configuration persistence, handler bodies and sensor APIs are
unchanged. Two WLAN debug-length arguments now use explicit unsigned casts to
match their format strings on both 32-bit ESP32 and 64-bit native builds.

Local library manifests explicitly declare the network/base/logging dependency
chain so transitive consumers build without relying on application includes to
expose dependency paths.

Validation on 2026-10-25:

- ESP32 application build succeeds on the local verification snapshot.
- Eight new native web tests cover independent route providers, CSS content and
  cache policy, offline 503, station/setup-AP serving, exact streaming length,
  rendering failure and application-independent/escaped page framing.
- Overall: 428 passed and the same pre-existing board-profile failure.
- Previously passing tests retain their outcomes.
- New web-host hardware runtime test remains pending; no upload was performed.

The next hardware smoke test should open status, sensor/configuration pages,
large display forms, logs/refresh and the firmware page, and confirm styling and
navigation. Shared network/log/OTA page modules and runtime lifecycle separation
are still future steps; this commit extracts the server/frame boundary only.

## Runtime confirmation after step 3

The user reported that the new web-host version looked good in the runtime
check. Further work continues on EnvNode alone; no test/provisioning application
is being built as part of this separation.

## Step 4: network and log administration pages

`EnvNodeWebAdmin` contains NetworkWebRoutes and LogWebRoutes. EnvNode WebService
registers these providers with its existing WebHost and implements the narrow
IWebPagePresenter and INetworkSaveObserver adapters. Public constructor arguments
remain unchanged.

NetworkWebRoutes reads/writes only INetworkSettings. IConfigurationService
inherits this interface, retaining existing implementations and its network
reference adapter. Save success/failure is reported back to EnvNode, which keeps
its previous ConfigurationArea::Network handling, pending restart action and
result texts. Network settings storage, keys, defaults and restore behavior are
unchanged.

LogWebRoutes uses only the log reader and web presentation interfaces. Log HTML,
JSON and refresh behavior are transferred unchanged. Existing routes/methods are
preserved across all providers (61 registrations in total).

Shared localization/catalog files move byte-for-byte into EnvNodeWeb; JsonWriter
moves into EnvNodeBaseConfiguration, keeping configuration backup and MQTT below
the HTTP layer. The translation inventory and browser test paths follow these
moves. No catalog entries or translations were changed.

Validation on 2026-10-25:

- ESP32 application build succeeds in the local verification snapshot.
- All previous native test outcomes are preserved.
- Seven new administration tests pass: form escaping/password secrecy, current
  locale/assigned addresses, preserving versus explicitly clearing passwords,
  static-field forwarding, failed-save notification, offline/setup-AP log JSON
  and localized log page/entry rendering.
- Overall: 435 passed and the same known board-profile assertion failed.
- Browser page-editor tests pass in English and German before and after the move.
- Source comparison confirms unchanged localization, log view and JSON helper
  bytes and unchanged URL/method inventory.
- No firmware upload was performed.

The next runtime check should open /network and /logs, exercise log refresh and
verify that a successful network save still reports the required restart without
performing it automatically. OTA/restart separation is the next independent step.
