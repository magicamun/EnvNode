# EnvNode

EnvNode ist eine modulare ESP32-Plattform für zuverlässige Umweltmessungen.
Mainboards, Erweiterungsmodule und Sensoren arbeiten mit einer gemeinsamen Firmware.
Messwerte werden über MQTT an externe Systeme weitergegeben, zum Beispiel Home Assistant.

## Neu bei EnvNode?

Mit EnvNode kannst du zum Beispiel Temperatur und Luftfeuchte messen, die Werte
im Browser ansehen und später an deine Hausautomation weitergeben. Du stellst den
Aufbau aus passenden Platinen und Sensoren zusammen und richtest ihn über die
Weboberfläche ein.

[Dein erstes EnvNode](getting-started/index.md){ .md-button }
[So arbeitet EnvNode zusammen](concepts/index.md){ .md-button }

Der Einstieg führt dich mit einem Mini und BME280 vom geprüften Aufbau bis zum
ersten lokalen Messwert. Die Gesamtübersicht erklärt Hardware, Firmware und den
Weg der Daten bis zu einer externen Anwendung.

## Direkt zu den Details

- [Kompatibilität](compatibility.md): passende Hardware und Firmware auswählen.
- [Mainboards](mainboards/index.md): Mini, Weatherstation und ORing Power.
- [Aufbauanleitungen](guides/index.md): Bestückung und elektrische Prüfungen.
- [Sensoren](sensors/index.md): unterstützte Treiber und Sensoranschlüsse.
- [Module](modules/index.md): Erweiterungen für zusätzliche Aufgaben.
- [Displays](displays/index.md): Anzeigen und deren Konfiguration.
- [Firmware](firmware/index.md): Einrichtung, Messbetrieb, MQTT und Diagnose.

## Umfang dieser Dokumentation

Die Dokumentation umfasst EnvNode Mini, Weatherstation, ORing Power,
AnalogHydroPressure, DuoRelay, SD-Card, die dreiteilige Testhardware, die Anbindung normaler I²C-Sensoren und die Firmware.
Mini, Weatherstation, ORing Power, DuoRelay und AnalogHydroPressure besitzen ausgearbeitete Produktseiten.
Sensor- und Displayübersichten sowie eine Firmware-Anleitung ergänzen die Produktseiten.

Die vollständige Station befindet sich noch im Aufbau. Bereits nutzbare Komponenten
werden deshalb einzeln beschrieben und nach ihrer jeweiligen Revision eingeordnet.
RainDetector und SCT013 sind ausdrücklich als Entwicklungsstand gekennzeichnet.
Weitere Entwicklungen werden erst nach Einzelfallprüfung aufgenommen.

[Erlaubte Nutzung und Lizenzbedingungen](license.md)

[Hardware-Schnittstellen und Pinbelegungen](interfaces/index.md)

[Dokumentationsstand und fehlende Anleitungen](guides/status.md) · [Alte Revisionen](guides/old-revisions.md)
