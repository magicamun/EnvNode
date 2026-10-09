# Firmware bauen und aufspielen

Das reguläre Build-Ziel ist ein klassischer ESP32 mit dem PlatformIO-Environment
`esp32dev`. Andere ESP32-Varianten dürfen nicht ungeprüft mit diesem Image verwendet werden.

## Voraussetzungen

- EnvNode-Repository und PlatformIO, etwa über die PlatformIO-Erweiterung in VS Code.
- Passendes ESP32-DevKit und ein USB-Datenkabel.
- Zum angeschlossenen Mainboard passende Board Identity beziehungsweise Boardkonfiguration.

USB-C am ORing-Power-Modul liefert ausschließlich Strom. Zum Programmieren den
USB-Anschluss des ESP32-DevKit verwenden.

## Build

Im Repository unter `firmware/`:

```sh
pio run -e esp32dev
```

Das Anwendungsimage entsteht unter:

```text
firmware/.pio/build/esp32dev/firmware.bin
```

Das Image ist für Web-OTA geeignet. Beim ersten USB-Flash übernimmt PlatformIO
auch die weiteren erforderlichen Flashbestandteile; nicht nur ein OTA-Image
an eine beliebige Flashadresse schreiben.

## USB-Flash

Den tatsächlich angeschlossenen Port ermitteln und einsetzen:

```sh
pio device list
pio run -e esp32dev -t upload --upload-port <PORT>
pio device monitor -p <PORT> -b 115200
```

`<PORT>` durch den angezeigten Gerätenamen ersetzen. Lokal eingetragene Ports
in `platformio.ini` sind rechnerabhängig und kein allgemeiner Standard.

## Nach dem Start

Ohne eingerichtetes WLAN startet die Firmware einen Setup-Access-Point mit dem
Namen `<Hostname>-Setup`. Verbinde dich damit und öffne die Weboberfläche unter
der Geräte-IP, die über die serielle Diagnose ermittelt werden kann.
Nach der Netzwerkeinrichtung wird die zugewiesene WLAN-IP verwendet.

[Weiter zur Einrichtung](configuration.md)
