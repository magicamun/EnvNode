# Firmware bauen und aufspielen

Das reguläre Build-Ziel ist ein klassischer ESP32 mit dem PlatformIO-Environment
`esp32dev`. Andere ESP32-Varianten dürfen nicht ungeprüft mit diesem Image verwendet werden.

Fertige Anwendungsimages und die wesentlichen Änderungen stehen unter
[Downloads und Änderungen](downloads.md).

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

Das Anwendungsimage wird nach erfolgreichem Build automatisch in den sichtbaren
Ordner `firmware/bin/` kopiert:

```text
firmware/bin/envnode-<version>.bin
```

Für den separaten Tester `pio run -e testboard` verwenden; dessen Image liegt unter
`firmware/bin/tester-<version>.bin`. Ein erneuter Build derselben Version ersetzt
die jeweilige Datei. Auch wenn das Image bereits aktuell ist, wird die Kopie erneuert.
Die Zwischenprodukte bleiben zusätzlich unter `.pio/build/<environment>/`.
Diagnosevarianten werden nicht in den OTA-Ordner exportiert.

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
Namen `<Hostname>-Setup`. Verbinde dich mit diesem WLAN und öffne
**http://192.168.4.1/** im Browser. Dies ist die Standard-IP im Access-Point-Modus.

Der Setup-Access-Point startet auch bei bereits gespeicherten WLAN-Zugangsdaten,
wenn die Verbindung beim Start nicht innerhalb des Verbindungszeitlimits gelingt,
zum Beispiel wegen eines nicht erreichbaren Routers oder falscher Zugangsdaten.
Die gespeicherten Einstellungen sind dadurch nicht automatisch gelöscht.
Über den Setup-Zugang kannst du sie prüfen und korrigieren.

Nach erfolgreicher Verbindung mit deinem WLAN wird die dort zugewiesene Geräte-IP
verwendet. Bei einem späteren Verbindungsverlust versucht die aktuelle Firmware
fortlaufend eine Wiederverbindung; dieser Laufzeitfall ist vom Start-Timeout zu unterscheiden.

[Weiter zur Einrichtung](configuration.md)
