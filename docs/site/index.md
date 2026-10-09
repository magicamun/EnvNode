# EnvNode

EnvNode ist eine modulare ESP32-Plattform für zuverlässige Umweltmessungen.
Mainboards, Erweiterungsmodule und Sensoren arbeiten mit einer gemeinsamen Firmware.
Messwerte werden über MQTT an externe Systeme weitergegeben, zum Beispiel Home Assistant.

## Hier anfangen

- [EnvNode Mini](mainboards/mini/index.md): kompaktes Mainboard, Anschlüsse und erste Inbetriebnahme.
- [Mainboards](mainboards/index.md): Mini, Weatherstation und ORing Power im Überblick.
- [Sensoren](sensors/index.md): unterstützte Treiber, Anschluss und eigene Sensorhardware.
- [Displays](displays/index.md): unterstützte Anzeigen und Konfiguration.
- [Firmware](firmware/index.md): Einrichtung, Messbetrieb, MQTT und Diagnose.
- [Bausätze und Aufbau](kits/index.md): von der Bestückung zur ersten Messung.

## Umfang dieser Dokumentation

Die erste Fassung umfasst EnvNode Mini, Weatherstation, ORing Power,
AnalogHydroPressure, DuoRelay, die Anbindung normaler I²C-Sensoren und die Firmware.
Mini, Weatherstation, ORing Power, DuoRelay und AnalogHydroPressure besitzen ausgearbeitete Produktseiten.
Sensor- und Displayübersichten sowie eine Firmware-Anleitung ergänzen die Produktseiten.

Die vollständige Station befindet sich noch im Aufbau. Bereits nutzbare Komponenten
werden deshalb einzeln beschrieben und nach ihrer jeweiligen Revision eingeordnet.
RainDetector und SCT013 sind ausdrücklich als Entwicklungsstand gekennzeichnet.
Weitere Entwicklungen werden erst nach Einzelfallprüfung aufgenommen.

[Wie EnvNode aufgebaut ist](concepts/index.md)

[Erlaubte Nutzung und Lizenzbedingungen](license.md)
