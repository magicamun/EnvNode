# Development

## Web firmware update

Build the firmware with PlatformIO from the `firmware` directory:

```sh
pio run
```

The resulting Web-OTA image is located at:

```text
.pio/build/<environment>/firmware.bin
```

Open the WeatherStation Web Administration Firmware page and select that
`firmware.bin` file. A successful upload stages the image but does not restart
the device. Use **Restart Now** to activate it, or leave the current firmware
running and restart later.

Web OTA accepts firmware application binaries only. USB flashing remains the
recovery path if a valid firmware cannot be uploaded through the running device.
