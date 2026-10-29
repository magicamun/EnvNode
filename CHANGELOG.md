# Changelog

## Unreleased

### Added

- EnvNode 0.8.4 / Testboard 0.8.1: move provisioning HTTPS downloads and
  SHA-256 verification into the browser, load its client with SRI, and validate
  bounded local uploads before the existing EEPROM write. Recover normal
  firmware flash by removing the provisioning TLS transport; preserve local
  forms and the separate tester connection-profile downloader.

- EnvNode 0.8.3 / Testboard 0.8.0: load one shared, hash-bound provisioning
  catalog for boards and modules over verified HTTPS. Preserve the separate
  tester menu and EnvNode hardware administration; show images and minimum
  runtime versions without gating EEPROM writes on driver support.
- Persist self-contained board port maps in verified ENPM dual-bank storage,
  preserve legacy ENID identities, and reserve module resources even when the
  installed firmware cannot operate the module. Keep built-in provisioning
  profiles available offline and derive the DuoRelay fallback from the same
  source as its downloadable template.

- EnvNode 0.8.2 / Testboard 0.7.0: share manufacturing revision metadata, support
  Mini 0.9 and Weather 0.3 beside their predecessors, select the exact EEPROM
  revision at startup and provide explicit revision-specific provisioning forms.
  Preserve numeric-ID API defaults and configuration; show the matching board
  image in tester provisioning without writing on selection.
- Testboard 0.6.0: add 40-input Mini 0.9/Weather 0.3 test profiles, verified HTTPS
  remote catalogs and compact profiles, built-in fallback and browser-loaded SVGs.
- Testboard 0.5.0: verified external board provisioning on dedicated GPIO16/5.
- Testboard 0.4.0: split-fixture routing derived from PCB data and 40-input support.

- Split the fixture hardware into Test-Controller, Input-Adapter and Module-Adapter,
  with verified cable/GPIO/multiplexer contracts in their READMEs. Retire the
  monolithic Test-Provisioning project; preserve its final state in Git history.
  Support 38 socket contacts plus two independent protected reference inputs.

- Testboard 0.3.2: omit component values from rendered board views while keeping
  reference designators; record centrally downloadable profiles as future work.

- Testboard 0.3.1: remove automatic Check all simulation and its API action;
  retain operator-confirmed connector tests and Mini PCB 0.8.1.

- Testboard 0.3.0: Check all for a complete simulated run, with automatic virtual
  reconnection, fixed fault selection, abort and preserved incomplete-coverage results.
- Testboard 0.2.2: add Mini 0.8 / PCB 0.8.1 with revision-specific mapping, drawing
  and explicit Net-Tie resolution.

- Testboard 0.2.1: use the connector list to select and inspect any connector
  whenever measurement is idle, retain full per-connector matrices, and retest
  individual connectors without clearing other results. Lock selection while
  running and let Next continue with the next untested connector.

- Guided simulated connector testing in testboard firmware 0.2.0: load Weather
  0.2 / PCB 0.2.1, confirm one pigtail connection at a time, compare each contact
  against all 38 socket pads and show board LEDs plus a full result matrix.
  Derive direct-net expectations and positions reproducibly from KiCad. Mark
  unmatched +3V3_SYS contacts as incomplete coverage instead of passing them.

- Independent `testboard` firmware build (initial version 0.1.0), with shared
  network/setup, device/time/locale settings, logs, OTA and explicit restart.
  Keep fixture settings/reset separate from EnvNode and select each application's
  sources, dependencies and version header independently. Connector test and
  EEPROM-provisioning engines remain future work.

### Documentation and repository maintenance

- Remove the former central parts spreadsheet, legacy BOM files and local order
  records. Article master data, purchasing, inventory and kit BOMs are maintained
  in an internally used solution.
- Keep migration scripts and internal inventory working files outside version
  control. Publish the user-facing connection-set wiring instructions in the
  hardware assembly documentation.

## 0.8.0 — 2026-10-26

### Changed

- Separate the existing EnvNode application from shared libraries for logging,
  base settings and persistence, WiFi, time/locale, web administration, OTA/restart,
  identity/provisioning, I2C transport and firmware startup/service sequencing.
- Keep EnvNode board policy, sensor/actuator/controller behavior, MQTT integrations
  and complete configuration codecs in the application. The future test-board
  firmware can reuse the shared basis; it is not part of this release.
- Preserve existing configuration keys and backup formats. Add a dedicated
  256 KiB configuration NVS partition with verified migration on confirmed restore.
  Existing installations need one normal USB upload including the partition table
  before using the dedicated storage; no full-flash erase is required. Bootloader,
  original NVS and OTA application addresses remain unchanged.

### Fixed

- Bound and preallocate configuration uploads, reduce preview memory usage, and
  use checked exact-size snapshot/readback buffers to prevent restore allocation
  aborts. Failed staging retains the previous configuration.
- Log restore validation, encoding, storage, verification and activation stages.

### Validation

- ESP32 build and both browser-language checks pass. Native regression outcomes:
  515 passing tests and one pre-existing board-profile-count assertion failure.
- EnvNode Mini 0.8 runtime confirmed: startup and normal operation, configuration
  restore/restart, locale and timezone, and final startup/main-loop extraction.

## 0.6.2 — 2026-10-15

### Fixed

- Apply configured presentation units to display output, page previews and source
  labels, including hPa/kPa for local and sea-level pressure. Share unit resolution
  with Web and MQTT while preserving canonical readings and controller inputs.
- Keep page format strings and saved configurations compatible; numeric precision
  and literal unit labels remain controlled by each page's format strings.

### Validation

- 49 targeted native tests passed and ESP32 firmware built successfully.

## 0.6.1 — 2026-10-14

### Fixed

- Apply the configured Locale to GeoPosition and pressure-temperature age inputs,
  restore previews and reduction-age diagnostics. German uses decimal commas;
  English uses decimal points. Keep storage, backups and MQTT locale-neutral.
- Document GeoPosition and sea-level pressure in the German firmware guide.

## 0.6.0 — 2026-10-14

### Added

- Optional latitude, longitude and actual sensor altitude, with zero altitude
  distinguished from missing configuration.
- A separate derived sea-level pressure channel for physical and simulated
  barometers, available through Properties, display selection, MQTT and
  Home Assistant discovery while retaining local atmospheric pressure.
- Selectable local outdoor-temperature input with configurable freshness
  checking, temperature-aware reduction and standard-atmosphere fallback.
- Reduction diagnostics identifying method, temperature source, sample age
  and fallback or quality reason.

### Changed

- Persist location and temperature-source settings in NVS and include them
  in the Web configuration and backup/restore preview. Backup/snapshot
  format 3 retains compatibility with versions 1 and 2.
- Keep derivation separate from measurement quality: fresh, good temperature
  preserves pressure quality; missing or restricted temperature estimates
  good pressure without upgrading restricted pressure quality.

### Validation

- 398 native tests passed, ESP32 firmware built successfully, and the new
  functionality was verified on the device.

## 0.5.1 — 2026-10-13

### Fixed

- Keep the Device page's Hardware and Provisioning button below the hardware
  details with consistent spacing, preventing overlap with the serial number.
- Correct the timezone documentation link on the published setup page.
