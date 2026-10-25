# Development

## Local serial ports

Installation-specific upload and monitor ports belong in the optional, Git-ignored
`firmware/platformio.local.ini`, which PlatformIO loads automatically:

```ini
; EnvNode: see ../LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
[env:esp32dev]
upload_port = /dev/cu.usbserial-1
monitor_port = /dev/cu.usbserial-1
```

Replace the example ports with the local device ports. The serial-diagnostics
environment inherits these settings. Without this file, PlatformIO uses its
default port detection. Keep this file limited to local connection settings;
shared build settings remain in the tracked `platformio.ini`.

The firmware build's `dirty` flag covers uncommitted changes under `firmware/`,
including `platformio.ini`, but excludes Git-ignored files such as this local
configuration and generated build output. Hardware, PCB and parts-list changes
outside `firmware/` do not affect the flag.

## Web firmware update

Build the firmware with PlatformIO from the `firmware` directory:

```sh
pio run
```

The resulting Web-OTA image is located at:

```text
.pio/build/<environment>/firmware.bin
```

Open the EnvNode Web Administration Firmware page and select that
`firmware.bin` file. A successful upload stages the image but does not restart
the device. Use **Restart Now** to activate it, or leave the current firmware
running and restart later.

Web OTA accepts firmware application binaries only. USB flashing remains the
recovery path if a valid firmware cannot be uploaded through the running device.

## Shared firmware basis: logging

The first shared component lives in `firmware/lib/EnvNodeLogging`, as a local
PlatformIO library. Both the ESP32 application and native tests consume the same
sources. Existing includes such as `Logger.h` and `StructuredLogger.h` remain
unchanged. Logging implementation files are no longer enumerated in the native
application source filter.

Application composition remains in `firmware/src/main.cpp`. The system-time
adapter and web log view stay in the application for now. Network configuration,
OTA, the central WebService and sensor APIs have not been changed. A second
firmware application is deliberately deferred until the common basis is extracted.

For the first extraction, compare the existing application build and all native
tests before and after the move:

```sh
cd firmware
pio run -e esp32dev
pio test -e native
```

A hardware smoke test still needs to cover boot, serial logging, WiFi/AP fallback,
configuration persistence, web pages, measurements/MQTT and OTA on an existing
EnvNode board. A successful host build does not establish these hardware results.

## Shared network and configuration types

`firmware/lib/EnvNodeNetwork` contains WiFiService and its interface. Its only
configuration dependency is INetworkConfiguration from
`firmware/lib/EnvNodeBaseConfiguration`; a minimal application need not construct
EnvNode sensor/actuator configuration to start WiFi.

EnvNode Configuration.h still exposes the same field names and types, importing
the shared base definitions. IConfigurationService forwards the narrow network
view to its existing Configuration.network member. NVS storage and restore
formats remain unchanged. See FirmwareBasisRefactoring.md for the scope and
validation record.

## Shared web host

`firmware/lib/EnvNodeWeb` owns WebServer, the common page frame and stylesheet.
Applications register IWebRoutes providers with WebHost before starting it. The
current EnvNode WebService remains the application-level provider; existing URLs
and pages are retained. Rendering takes the device/product metadata and trusted
application fragments explicitly rather than depending on EnvNode Configuration.

Changes in the shared header/streaming path need a browser smoke test as well as
the native web-host tests, especially for large display forms. Existing runtime
behavior before step 3 was confirmed by the user; this web change requires its
own hardware check.

## Administration route modules

EnvNode WebService registers NetworkWebRoutes, LogWebRoutes and FirmwareWebRoutes from
`firmware/lib/EnvNodeWebAdmin`. These modules use a page presenter and narrow
settings/log interfaces rather than the complete application runtime. EnvNode
still owns the result-page and restart decision after a network save.

Shared translations are in `firmware/lib/EnvNodeWeb`; their public include names
are unchanged. The JSON helper is in EnvNodeBaseConfiguration and remains
independent of HTTP. Native administration tests run with `pio test -e native
-f test_web_admin`; browser script tests run from the repository root with
`node firmware/test/browser/page_editor_test.cjs`.

## OTA and restart boundary

The shared OTAService and platform restart executor are in
`firmware/lib/EnvNodeLifecycle`. OTA requests restart through IDeviceRestart;
EnvNode RuntimeManager retains pending-action priority and explicit execution.
No new automatic restart is introduced. Firmware HTTP handlers live in EnvNodeWebAdmin; factory-reset storage policy
remains application-owned through an injected callback. Native lifecycle tests run with
`pio test -e native -f test_ota_lifecycle`; physical OTA/boot needs its own runtime
check.

## Identity and provisioning storage

`firmware/lib/EnvNodeIdentity` contains the portable identity/descriptor codecs,
CRC, storage interfaces, verified board provisioning and redundant descriptor
bank store. Its ten implementation files are compiled as a local library in
both native and ESP32 environments. Existing header names remain valid; board
IDs and revisions are now declared in BoardIdentityTypes.h and included by
BoardProfile.h. EEPROM wire transport, board fallback/selection, module discovery
and compatibility policy remain application services.

The existing native suites test_board_identity_codec, test_board_provisioning,
test_hardware_descriptor_codec, test_hardware_descriptor_store,
test_module_discovery and test_module_descriptor_provisioning cover this
boundary. The library also compiles and links using only its own include
folder and standard C++11, without application headers or Arduino.

## Fixed I2C wiring and EEPROM transport

`firmware/lib/EnvNodeI2C` owns I2CBusController, the II2CBusProvider interface,
bus types and IdentityEeprom24LC32. Each application supplies its fixed bus/SDA/SCL
constants during startup. No pin fields were added to web forms or NVS.
EnvNodeIdentityI2C in `firmware/include/EnvNodeI2CConfiguration.h` remains I2C0
on GPIO21/22; I2CBusManager is the EnvNode adapter for bootstrap and board-profile
startup/scanAll. Normal I2C1 remains GPIO25/26 in the current board profiles.

The EEPROM constructor accepts a provider, device address and optional bus ID.
Existing calls default to I2C0; main.cpp passes EnvNodeIdentityI2C.bus explicitly.
The test application will supply its own fixed wiring, without a new EnvNode
board profile or user-selectable pin configuration.

Native bus/EEPROM checks run with `pio test -e native -f test_i2c_transport`.
The Wire double emulates EEPROM page wrap to verify page splitting and exposes
bus initialization, address transactions, NACKs, short transfers and ACK timeouts.
