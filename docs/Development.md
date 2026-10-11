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
