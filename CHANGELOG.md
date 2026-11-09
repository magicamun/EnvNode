# Changelog

## Unreleased

- ORing Power 0.1.2: C2 system reservoir changes from 10 µF to 47 µF / 6.3 V,
  Nichicon UWG0J470MCL1GB (DigiKey 493-9737-1-ND), retaining CP_Elec_5x5.4.
  Correct the interchanged C2/C4 functional descriptions: C4 is the existing
  47 µF tantalum at the LM3940 output. Signal routing and board dimensions are unchanged.
  J1 mounting pads are now explicitly grounded in schematic and PCB; silkscreen
  clearance is corrected. Procurement record and InvenTree article ENP-00179
  added with manufacturer and supplier linkage.


### Added

- EnvNode 0.8.8 / Testboard 0.13.0: integrate tester measurements, fixture profiles,
  diagnostics and report history with current main provisioning/dialog/profile core.
  Include reviewed Controller 0.2 / ESP32 adapter 0.3 / module adapter 0.2 CAD and
  retain produced Legacy hardware; physical acceptance of new revisions is pending.

- Derive dew point and absolute humidity only from paired temperature/humidity
  values of one sensor instance and acquisition. Add a configurable virtual
  Solar Position slot using location and UTC, with geometric azimuth/elevation.
  Publish all four channels through MQTT, Home Assistant Discovery and properties.
  Expand implementation metadata to six channels and discovery masks to 32 bits.

- EnvNode 0.8.6 / Testboard 0.8.3: display freshly read board/module EEPROM
  identity and manufacturing fields beside the planned write. Add explicit
  refresh/adoption and a before/after summary. Re-read and compare the complete
  selected EEPROM before dialog writes; refuse changed/unavailable targets with
  HTTP 409, retain entered data and require review and confirmation again.


- EnvNode 0.8.5 / Testboard 0.8.2: share one provisioning dialog with filtered
  Board/Module selection, matching image, common serial/batch/date fields and
  explicit target-specific write action. Merge matching online and built-in
  profiles in one list while retaining offline provisioning and legacy POST APIs.
- Store board batch/date in ENPM payload v2 with verified readback; retain v1
  reads and byte-identical v1 writes without batch/date. Report the effective
  minimum runtime version 0.8.5 when board manufacturing extensions are used.


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
- Testboard 0.12.1: sort provisioning board choices by name and numeric revision,
  preserving profile IDs, defaults and the currently selected identity.

- Testboard 0.12.0: select and persist independent Controller 0.1/0.2 and Legacy/
  ESP32 DevKitC 0.3 adapter profiles. Derive mapping from PCB, preserve produced
  hardware, adapt guided TP2 self-tests and isolate results/history by fixture.

- Testboard 0.11.0: download complete board-session JSON reports with bounded,
  read-only paging; retain the latest 128 unexpected/open/error observations with
  actual LOW/HIGH samples, reason, attempt and uptime across successful retests.
  Include profile, directed evidence, all matrices, build identity and eviction counts.

- Testboard 0.10.1: validate Weather 0.3 GND→SYS reference conduction using
  SYS pigtail excitation for the reverse check and mandatory positive reference
  continuity. Keep shorts and open reference cables detectable; scope the new
  rule to the measured revision and expose the actual reverse source in the UI.

- Testboard 0.10.0: distinguish directed EEPROM GND→I2C0 paths from shorts;
  require reverse isolation and positive connection evidence, recompute dependent
  results on retest and show D?/D/S in the matrix. Bind rules to reviewed electrical
  revisions without changing published profiles. Use validated 100 ms settling
  for real board tests; add directed-short simulation and regression coverage.

- Testboard 0.9.3: place guided self-test start/stop, confirmation and settling
  controls beneath the selected contact; keep detailed readings locally collapsible
  and preserve settings while moving through the list.

- Testboard 0.9.2: make guided self-test settling selectable at 15/50/100 ms
  and retain it per result for controlled comparisons; keep eight samples and
  unchanged thresholds/peak-based decisions.

- Testboard 0.9.1: identify the worst isolation channel/window, retain its raw
  samples and aggregate exceedance counts, and show manual meter-check settings
  without changing thresholds, timing or pass/fail decisions.

- Testboard 0.9.0: connect board tests to a real LOW/HIGH ADC fixture driver;
  preserve explicit simulation, require physical-target confirmation and clear
  results on mode change. Share electrical switching with hardware self-tests.
  Document EnvNode and Test/Provisioning in separate firmware manual sections.

- Testboard 0.8.0: add guided physical loopback commissioning for 22 outputs and
  40 original-adapter inputs, automatic LOW/HIGH/isolation measurements, free
  contact selection and a downloadable browser-session report.

- Testboard 0.7.2: port the verified main HTTP client gate and nonblocking
  response pump to the tester; avoid copying diagnostic page content.

- Testboard 0.7.1: add opt-in real controller diagnostics for mux address/enable
  selection, GPIO17 LOW/HIGH and GPIO34 ADC readings; disable after 60 seconds
  or network loss and block provisioning/test actions while active.

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

- Add versioned EnvNode and Tester OTA downloads to the documentation, with
  curated release summaries, commit identity and SHA-256. Keep the latest build
  per target/version in GitHub Releases and name local images by target/version.
  Export normal local OTA builds to the visible firmware/bin directory.

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
