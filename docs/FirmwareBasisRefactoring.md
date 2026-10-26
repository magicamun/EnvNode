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

## Runtime confirmation after step 4

The user confirmed the administration-page version worked. The refactoring
continues to preserve EnvNode as the sole firmware application.

## Step 5: OTA and explicit device restart

`EnvNodeLifecycle` contains OTAService, IDeviceRestart and DeviceRestartExecutor.
OTA no longer includes RuntimeManager or any sensor/actuator/controller runtime.
The upload algorithm is byte-for-byte equivalent after substituting only the
restart interface calls.

RuntimeManager implements IDeviceRestart while retaining action priorities and
its existing performPendingRestart() entry point. A shared executor owns the
only actual ESP.restart() call; RuntimeManager still gates it on RestartDevice.
Its constructor gains an optional last driver parameter for host tests; existing
construction sites do not change. The default native driver remains a no-op.

EnvNode firmware-page/upload handlers stay in WebService for this step. Explicit
restart, restore and factory-reset requests now use the narrow restart interface.
Their HTTP responses and client flush still precede execution, and busy-OTA
checks remain unchanged. Factory-reset storage behavior, partitions and NVS keys
are unchanged.

Validation on 2026-10-25:

- ESP32 application build succeeds.
- All previous native test outcomes are preserved.
- Ten new lifecycle tests pass: successful known/unknown-size staging, no restart
  during staging, empty/truncated uploads, partial writes, initialization retry,
  finalization/driver errors, abort/reject guards, EnvNode restart prioritization
  and integration of OTA staging with the EnvNode restart adapter.
- Overall: 445 passed and the same pre-existing board-profile failure.
- No device upload, erase or restart was performed by the agent.

The next hardware check should stage a valid image using Web OTA, choose restart
later and confirm the running firmware remains active, then explicitly restart
and verify boot/configuration. Native Update tests verify interaction/state
policy, not physical flash integrity, bootloader behavior or rollback.

## Step 6: shared firmware administration

The user confirmed the step-5 firmware on hardware, including OTA staging,
restart later, explicit restart and preserved configuration.

FirmwareWebRoutes in EnvNodeWebAdmin now owns the firmware page, multipart OTA
handlers, explicit restart and factory-reset HTTP routes. The existing public
WebService constructor, URLs, form fields, page content and OTA state machine
are preserved. Firmware metadata is supplied as data by the application;
the shared module does not include the generated application build header.

Two callbacks keep application-specific storage/reset and number formatting
behind the EnvNode adapter. The shared routes depend on IDeviceRestart and
OTAService rather than RuntimeManager, IConfigurationService, sensors or NVS.
Reset still calls the existing resetToDefaults implementation. Upload success
still stages only; an explicit restart sends the response and flushes the client
before invoking the restart interface. Configuration-restore handling remains
in the EnvNode application and retains its OTA busy checks.

Eight new native HTTP regression tests cover route/page metadata, response and
flush ordering, successful/failed reset, busy-upload guards, successful staging
without execution of a restart, invalid file rejection and aborted uploads.
Browser checks include the newly moved confirmation strings in both languages.
No device upload or reset was performed by the agent during this step.

Validation on 2026-10-25: ESP32 build succeeds; browser checks pass in English
and German; translation inventory reports zero missing or unwired entries.
All previous native outcomes are preserved. The first full run exposed a test
assertion with incorrect capitalization of a localized label; after using the
catalog text, all eight new tests pass in the focused rerun. Together this gives
453 passing assertions/tests and the same existing board-profile failure.
The registered route/method inventory is unchanged.

## Step 7: identity and provisioning storage basis

EnvNodeIdentity owns board identity records, validation, codecs, CRC32, board
provisioning with verified readback, hardware descriptor models/CBOR codecs and
the redundant descriptor-bank store. It depends only on the C++ standard
library and IIdentityStorage; it has no Arduino, Wire, sensor, network, HTTP,
NVS, active-board selection or application runtime dependencies.

BoardProfileId and BoardRevision are shared data types in BoardIdentityTypes.h;
BoardProfile.h includes that header, preserving callers and encoded values.
BoardIdentity.h includes the data types directly rather than board runtime
capabilities. Existing public include names and provisioning APIs are retained.
All ten moved implementations remain byte-for-byte identical. Supported board
identities/revisions and record formats remain unchanged.

BoardIdentityResolver, the I2C/EEPROM transport, module discovery, compatibility
policy and ModuleDescriptorProvisioningService remain in the EnvNode application.
They decide how a stored identity affects the running device. No new tester
application, EEPROM write operation, configuration migration or firmware upload
is part of this step.

Validation on 2026-10-25:

- All ten moved implementations match their pre-extraction contents byte-for-byte.
- All shared sources compile with C++11, -Wall -Wextra -Werror and only their own
  include directory; linking all objects succeeds without application/Arduino.
- ESP32 application build and English/German browser tests pass.
- Native full run preserves all earlier passing results except a compile error
  in the board-provisioning integration test: it had obtained BoardProfile.h
  transitively. Adding its explicit include restores all seven tests in a focused
  rerun. Combined results are 453 passing tests with the same pre-existing
  board-profile-count failure; no product assertion or profile data was changed.
- No physical EEPROM write, device restart, flash or configuration migration
  was performed. A physical EnvNode runtime check remains for the user.

## Step 8: fixed application I2C wiring and shared EEPROM transport

The user confirmed Mini 0.8 provisioning and normal runtime for step 7.

EnvNodeI2C owns bus initialization/availability/scans and the 24LC32 EEPROM
transport. Applications supply fixed bus/pin definitions in code. There are no
new web settings, NVS fields or user-selectable pin maps. EnvNode's adapter
retains board-profile selection and scanAll; its bootstrap pins remain 21/22.
The future test application can initialize its own fixed bus through the same
controller without depending on EnvNode board resources. No test firmware or
unverified test-board pin assignment is introduced in this step.

The EEPROM driver depends on II2CBusProvider and an explicit bus ID (I2C0 is
retained as the compatible constructor default), not the EnvNode bus manager.
EnvNode explicitly passes its fixed identity bus to all three EEPROM objects.
Capacity, addressing, page/chunk sizes, timeout and read/write sequence remain
unchanged. The controller rejects attempts to remap an initialized bus rather
than silently using the previous pins. Repeating the same assignment is a no-op;
a failed initialization can be retried. Bootstrap success/failure logs now use
the same generic format as ordinary bus initialization.

Validation on 2026-10-25:

- ESP32 application build passes; English/German browser checks pass.
- Full native run: 462 passing tests, the same pre-existing board-profile-count
  failure, no compilation errors or additional failures.
- Nine new native tests exercise fixed pin/bus selection and idempotency,
  rejection of remapping/invalid bus IDs, failed-init retry, unchanged EnvNode
  bootstrap/profile wiring, EEPROM page/read-chunk boundaries, independent bus
  and device addresses, bounds/unavailable bus guards, short transfers, NACK/
  ACK timeout and scan address/error reporting.
- No device upload, physical EEPROM write, configuration erase or restart was
  performed. The next hardware check should confirm identity readback, normal
  sensor/I2C operation and Mini 0.8 provisioning followed by restart.

## Step 9: provisioning web routes and application policy adapter

The user confirmed the shared I2C/EEPROM transport on hardware after step 8.

HardwareProvisioningWebRoutes owns /hardware and both existing aliases for board
and module writes. Forms, input parsing, confirmation fields, result messages,
DuoRelay descriptor encoding and UUID-v4 formatting remain unchanged. Applications
supply IProvisioningAdministration for profile metadata/current identity and the
actual provisioning operations; randomness and the read-only hardware overview
are injected separately. The module does not depend on BoardProfile,
ModuleDiscoveryService, RuntimeManager, IConfigurationService or ESP random APIs.

EnvNode retains its read-only runtime/compatibility/device-inventory overview,
active-profile selection, module compatibility checks, discovery and write policy.
Provisioning does not request or execute an automatic restart. Success still offers
restart now/later for board identities and preserves the module rediscovery path.
Pure module status types, slot IDs/names, UUID helper and deterministic DuoRelay
encoding move to EnvNodeIdentity; compatibility evaluation remains application-owned.
The public WebService and provisioning service constructors remain unchanged.

This closes the provisioning extraction block, not the overall application split.
Still to audit/extract: time and locale/settings service boundaries, device metadata
settings, application-specific configuration backup/import/export adapters and the
startup/loop composition. The tester application is not introduced in this step.

The remaining configuration boundary includes common settings persistence:
ConfigurationService still owns the complete EnvNode model and Preferences bank
selection. Existing shared data types/read-only interfaces alone do not yet make
an independent application configuration store. Backup/import/export must remain
application-specific through appropriate adapters.

Validation on 2026-10-25:

- ESP32 build passes. The first link exposed a 48-byte static DRAM overflow;
  placing the small provisioning route object behind an owned heap pointer at
  web startup resolves it. Allocation failure is checked/logged and does not
  disable the other administration routes.
- Full native suite: 471 passing tests, the same pre-existing board-profile-count
  failure, no new failures or compile errors. Nine new HTTP tests cover forms,
  locale/escaped profile names, real Mini 0.8 record write/readback without changing
  active identity, aliases/confirmations, invalid forms, write failure, valid
  module CBOR/manufacturing/UUID-v4 data and policy/storage failure responses.
- English/German browser checks pass. Translation inventory reports zero missing
  or unwired entries. URL/method inventory is unchanged.
- The expanded identity library still compiles with C++11/-Wall/-Wextra/-Werror
  and links using only its own include directory and standard library.
- No physical EEPROM write, erase, upload or restart was performed by the agent.
  Hardware follow-up: open Hardware and verify inventory/forms; provision Mini
  0.8, choose restart later, then explicitly restart and check identity/runtime.
  Module HTTP coverage is native; physical module provisioning needs a suitable
  module and slot and is not covered by Mini board provisioning alone.

## Step 10: time, locale and basic device settings

The user confirmed provisioning-web extraction on hardware after step 9.

Read-only ITimeConfiguration and ILocaleConfiguration remove complete EnvNode
configuration dependencies from TimeService and LocaleFormatter. IBaseSettings
adds just device/time/locale settings access. IConfigurationService adapts the
existing model to these interfaces, retaining setter behavior, Preferences keys,
defaults, bank selection and caller construction sites. TimeService/ITimeService
move to EnvNodeTime; LocaleFormatter moves to EnvNodeBaseConfiguration.

CommonSettingsWebRoutes owns /time, /time/save, /device and /device/save. It uses
base settings, time service, formatter and page/header presenters. EnvNode keeps
configuration-save/runtime action decisions behind callbacks and provides the
pressure-reduction, geolocation and hardware-summary device-page sections.
The timezone selection is still streamed in bounded chunks, and custom values,
locale selection, offline/render failure guards, form fields and sequential
setter/partial-failure behavior are preserved. No settings migration or new
configuration fields are introduced.

Validation on 2026-10-25:

- ESP32 application build and English/German browser checks pass. The additional
  interfaces initially exceeded the constrained static DRAM segment by 16 bytes;
  the existing firmware route object now uses checked heap allocation at web
  startup, matching provisioning/settings routes. OTA/reset handlers and their
  response/flush ordering remain unchanged. Final static RAM usage is 124488 bytes.
- Full native run: 482 passing tests, the same pre-existing board-profile-count
  failure, no new failures or compilation errors. Eleven new tests cover WiFi/SNTP
  startup/defaults/restart order, fixed-timestamp timezone offsets, live DE/GB/US
  formatting, streamed/custom timezone options, escaped inputs, offline/setup-AP
  and failed-header paths, save change detection, invalid locale, sequential
  short-circuit failures, device-page extras and save-result delegation.
- The timezone catalog is byte-identical; its generator and documentation now
  use the library path. URL/method inventory is unchanged. Translation inventory
  reports zero missing or unwired entries.
- LocaleFormatter differs only in the injected locale interface. TimeService
  differs only in the narrow time-settings access and explicit system includes.
  Preferences code, stored schema/defaults and backup format are untouched.
- No device flash, setting change, reset or physical restart was performed.
  Hardware follow-up: check synchronized time and localized date/numbers, save
  locale/time and device name, restart and verify preserved values and normal
  sensor/runtime operation. The previous provisioning runtime was confirmed.

The remaining main boundaries are shared settings persistence and the application
startup/loop composition; application-specific backup/restore must stay adapted
to the appropriate configuration model. This step does not create test firmware.

## Step 11: reusable configuration storage and base settings persistence

The user confirmed time/locale/device administration on hardware after step 10.

EnvNodeConfigurationStorage owns Preferences lifecycle, typed key writes, opaque
snapshot staging/readback/metadata commit and erase across legacy/two-bank storage.
A fixed application-owned namespace layout is injected. EnvNode retains weather,
weather0, weather1 and weather_meta, active and cfg_snapshot keys and the existing
bank selection/commit order. No namespace/schema migration is introduced.

BaseSettingsService operates on injected device/network/time/locale references,
application defaults and the shared storage. It supplies common read/write,
validation/default recovery and legacy override loading without the EnvNode
Configuration model, sensors, MQTT, web or runtime dependencies. EnvNode's existing
ConfigurationService delegates its base operations using a lightweight temporary
adapter; its public API, in-memory model and application-specific slot/value/
display/MQTT handling remain unchanged. Snapshot encoding/validation and complete
EnvNode backup/import/export policy remain application-owned.

Initialization/override ordering, explicit empty-value tombstones after snapshots,
sequential partial-write behavior and in-memory updates only after success are
preserved. The store deliberately retains the existing post-commit reopen result
semantics; this refactoring adds no new transactional guarantee to ordinary edits.

Validation on 2026-10-25:

- ESP32 application build and English/German browser checks pass; static RAM is
  124496 bytes. No duplicate full configuration model or persistent base adapter
  object was added to the ESP32.
- Full native suite: 493 passing tests, the same pre-existing board-profile-count
  failure, no new failures/compilation errors. All 29 focused base/backup/storage
  tests pass, including eleven new storage tests for fixed namespace compatibility,
  independent base-settings persistence, exact snapshot bytes and alternating banks,
  failed staging/readback/metadata commit, initialization/clear failures and retry,
  empty restored overrides, validation/default recovery, partial-write semantics,
  static network/password retention and isolation between application layouts.
- A standalone C++11 native program links/runs with only the new storage library,
  shared base validation/types and platform doubles, without any EnvNode model or
  application header. It exercises base persistence, snapshot replacement and clear.
- Full EnvNode backup/record codec files are untouched. Existing restore/reboot,
  credentials, sensor/actuator/controller/display/value and reset tests remain green.
- No physical upload, write, reset or restart was performed. Hardware follow-up:
  export a backup, change/save a base setting, reboot and verify; restore the backup,
  reboot and verify base values plus sensors/slots. Factory reset is not required.

The remaining structural work is separating startup/main-loop composition and a
final dependency review. The existing EnvNode backup schema deliberately remains
application-specific; the future test application supplies its own record codec.

## Restore preview follow-up

After step 11 the user reported a reproducible hang while uploading/checking the
backup, before the preview page appears. They clarified that restore had not been
used for a long time, so the report does not establish a regression from the
latest extraction. This phase does not call configuration persistence.

Review found pre-existing memory pressure: an unchecked geometrically growing
JSON index vector alongside uploaded JSON, a destination Configuration and a
second decoded copy. The JSON index now uses a counted, exact-size nothrow array;
allocation failure returns false. The web adapter takes ownership of one decoded
candidate and releases the uploaded JSON before domain validation and rendering.
The legacy decoder interface delegates to the same codec. File versions and
credential/default/validation behavior remain unchanged; bank storage is untouched.

Upload/decode/validation/response INFO markers now show lengths, timing and heap
availability, without configuration values/passwords. Five new native tests cover
single checked candidate/index allocation, injected index-allocation failure with
candidate/credential flags unchanged (including the legacy API), excessive field
counts rejected before allocation, and equivalent owned/legacy decoding with
omitted-password retention. These verify the corrected allocation path, not yet
the exact cause or resolution of the user's physical device hang.

Validation: ESP32 build passes; English/German browser checks pass; full native
suite has 498 passing tests and only the same pre-existing board-profile failure.
All existing backup/restore compatibility cases pass. Translation inventory stays
at zero missing/unwired entries. No physical flash, reset or restore was performed
by the agent. The next device retry should first check whether the preview appears;
if it still hangs, the final Configuration backup/preview phase marker is needed.

## Dedicated configuration capacity follow-up

The user's next device log reached the confirmed restore and reported
nvs_set_blob cfg_snapshot NOT_ENOUGH_SPACE. Preview had progressed; the default
20 KiB NVS could not hold live data plus the staged snapshot and bookkeeping.
A local native default configuration measures 5172 compact bytes / 31009 JSON
bytes; actual device records vary. The user confirmed USB flashing is available.

The new table adds cfg_nvs at 0x390000 (256 KiB), retains the original NVS/OTA/
app locations and reduces unused SPIFFS to 0x3D0000 (192 KiB). Storage layouts
accept an optional target partition. EnvNode keeps legacy data authoritative until
a confirmed snapshot writes/verifies/commits in the dedicated partition; then
normal edits and reboot loading use it. The old source is retained on migration.
Missing tables allow legacy operation while restore is rejected. Reset clears
both configuration sources. Partition contents are never auto-erased on init error.

Six new native integration tests cover existing legacy settings after table-only
installation, actual full-model restore/reboot and edited keys in cfg_nvs, failed
metadata commit retaining legacy authority, old-table operation, old default-NVS
snapshot migration, bank alternation and reset across both partitions. The native
Preferences double now isolates partitions, and restore suites reset both sources.

Validation: ESP32 build and both browser-language checks pass. Full native run:
504 passing tests and only the same existing board-profile-count failure. The
partition CSV has no overlap, fits 4 MiB and preserves all original NVS/OTA/app
ranges; PlatformIO also builds its binary successfully. No physical flash or
erase was performed. Next: retain JSON backup, normal USB upload including table,
verify previous settings, then preview/confirm restore and reboot. Physical NVS
capacity and previous flash contents still require that hardware check.

## Upload reception and confirmed-restore trace follow-up (2026-10-26)

The user reported a stall on confirmation. Read-only live inspection showed the
node was still serving HTTP and running controllers. Export without passwords
succeeded (31722 bytes / 901 fields). Two preview-only uploads returned HTTP 400
after roughly 11/13 seconds, logging bytes=0/rejected=1 before decoding. No
confirmed restore, NVS write, reset or flash was performed by the agent. A local
native probe decoded and validated the real exported Mini 0.8 file successfully;
the file was not committed to the repository.

The upload now reserves the declared file size at START, using a query hint set
by the form before multipart processing. Length limits and final length are
checked; absent hints remain backward-compatible. A shared TextUpload helper
uses terminated bounded chunks rather than handing raw full upload buffers to
an Arduino concat implementation that copies length+1 bytes. The handler now
logs specific preparation/append/abort rejection reasons and heap figures.

ConfigurationService accepts an optional logger (existing default construction
remains valid). Main injects the current logger; restore and storage log each
validation/encoding/write/verify/commit/reopen/activation stage. Readback uses a
checked nothrow allocation instead of an unchecked vector allocation. No storage
layout, backup schema, settings or automatic-restart policy was changed.

The live failure establishes upload rejection, not the cause of every reported
confirmation stall. The allocation improvement and phase logs require a new
physical firmware run before the full user issue can be marked resolved.

Validation: ESP32 build and both browser checks pass. The full native run retained
505 previous/new passing tests and the known board-profile failure; the new upload
suite initially lacked native pinMode/digitalWrite stubs. After adding the same
stubs as the existing suites, all six upload tests pass. A final focused rerun of
all 29 affected upload/preview/storage/partition tests passes. Combined maintained
suite outcomes are 511 passes and the same pre-existing board-profile failure.
The additional live-export probe passes separately and is not a committed test.
No device firmware upload, confirmed restore, NVS write or restart was performed.

Restore runtime follow-up (2026-10-26): Build 378 preview succeeded, but confirmation aborted in compact snapshot vector growth before storage. The restore encoder now counts bytes and allocates a checked exact-size buffer. Compact bytes are verified against the existing codec. Snapshot-allocation and readback-allocation failure tests retain the active and persisted configuration. Validation: 26 relevant native tests pass; ESP32 build passes. Device restore/restart confirmation remains pending.


## Step 12: startup and main loop composition (2026-10-26)

The user confirmed successful configuration restore, restart, locale and timezone
with commit 1145e76. The restore blocker is resolved on the physical Mini 0.8.

EnvNodeFirmwareRuntime now owns the common startup/service order through narrow
WiFi, time and web interfaces and IFirmwareApplication. Configuration and board
identity must be ready before begin. Device initialization precedes web startup;
application integrations follow it. Each loop services WiFi/time, measurements,
web, then application integrations. Startup does not wait for network or time
synchronization. A loop before begin does nothing; a second begin does not restart
services. The application hooks retain the existing sensor/actuator/controller,
MQTT, discovery, restart and display ordering.

main.cpp now only delegates Arduino entry points. EnvNodeFirmware.cpp owns the
EnvNode composition, identity/profile gate, module discovery, configuration,
automatic actuator setup and sensor runtime construction. Object lifetime and
construction order remain static and unchanged. Application's public setup/loop
API remains available. The tester can supply its own composition and hooks without
linking the EnvNode model, sensors, MQTT integrations or module policy.

Dependency review: all eleven shared libraries were checked for includes of
application headers, declared direct shared dependencies and cycles. None includes
an application header; all observed direct dependencies are declared; no cycles
were found. Full configuration/backup codecs, board profiles, module compatibility,
sensor/controller policy, MQTT and application route assembly intentionally remain
in the EnvNode application. No tester application is introduced in this step.

Runtime acceptance still requires flashing this step and checking existing startup,
measurements/controllers, MQTT, web and display. After that the initial extraction
is complete; future tester-specific adapters/build environment are separate work.


Validation of step 12: ESP32 build and both browser-language checks pass. The full
native run has 512 passing tests and the existing profile-count failure; the new
lifecycle suite initially missed the GPIO stubs required by the native application
source set. With those supplied all three lifecycle tests pass (515 maintained
passing outcomes in total, one known profile-count failure). The same three tests
also compile and run standalone with only shared interface headers, runtime source,
Unity and platform doubles, without application headers/sources. A mechanical
comparison confirms unchanged composition object order and bootstrap/loop bodies.
ESP32 static RAM increases by 16 bytes and flash by 500 bytes versus the preceding
restore fix. No device was flashed or restarted by the agent.


## Runtime acceptance and integration release

The user confirmed normal device operation after step 12 (399d102). Together with
successful restore/restart and locale/timezone acceptance, the initial shared-basis
extraction is complete. Release 0.8.0 integrates this work into main before further
SD-Card or tester firmware development. The test-board firmware remains separate
future work. Existing branch-local uncommitted PCB, binary and ignore-file changes
are not part of the integration release.


## First separate fixture application (2026-10-26)

After release 0.8.0/main integration and SD-Card synchronization, the fixture gets
its own `testboard` build target and source directory. EnvNode's existing target,
version, configuration and behavior remain separate. Common ESP32 build/monitor
settings are shared in PlatformIO; sensor and MQTT dependencies belong only to
esp32dev. The generator selects version headers per environment: EnvNode 0.8.0,
fixture 0.1.0. Default builds still select EnvNode explicitly through default_envs.

The fixture composes shared services for WiFi setup, time/locale, base settings,
logging, web administration, OTA/explicit restart and lifecycle. Its NVS namespaces
and reset are independent of EnvNode's weather namespaces and cfg_nvs snapshots.
Seven tests in testboard_native cover isolation, persistence/reboot/reset, actual
route composition in setup mode, live locale, save failures and explicit restart,
including firmware staging without automatic restart. No measurement/provisioning
engine or I2C/MUX/ADC/DUT pin activation is introduced. Hardware follow-up remains
pending; see TestBoardFirmware.md.


Validation: both ESP32 targets build, browser checks pass in English/German, and
all seven isolated fixture tests pass. EnvNode retains 515 passing native outcomes
and the same known profile-count assertion failure. Generated headers select the
correct application versions. The tester ELF has no symbols for EnvNode's
ConfigurationService, SensorManager, ControllerRuntime, MqttService, DisplayService,
BoardIdentityResolver, PubSubClient or U8G2. Initial measured footprint is about
974 kB flash / 65 kB static RAM for the tester versus 1462 kB / 125 kB for EnvNode.
No physical flash, restart or connected-board operation was performed.
