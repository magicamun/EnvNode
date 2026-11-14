# Dokumentationsstand

Stand: **14. November 2026**. „Abgeschlossen“ bezeichnet die redaktionell vollständige Aufbau- und Bring-up-Anleitung. Der reale Fertigungs-/Prüfstand steht jeweils in der Anleitung.

## Abgeschlossen

- EnvNode Mini 0.10.0 und Weatherstation 0.4
- ORing Power 0.1.2 und DuoRelay 0.3
- AnalogHydroPressure 0.6
- RainDetectorController 0.4 und RainDetectorHead 0.2
- Test-Controller 0.2, ESP32 DevKitC Adapter 0.3 und Modul-Adapter 0.2

[Zu den Anleitungen](index.md).

## Revisionsbindung und Abnahme

Die aktuellen Anleitungen und PDF-Ausgaben dokumentieren den jeweiligen
CAD-Stand. Die bisherigen Anleitungen bleiben unter „Alte Revisionen“ erhalten.
Für Mini 0.10.0, Weather 0.4, Test-Controller 0.2, DevKitC Adapter 0.3 und
Modul-Adapter 0.2 steht die physische Abnahme am gefertigten Aufbau aus.
Die revisionsgebundene PCB-Artikelzuordnung ist in den Stücklisten noch zu prüfen;
bekannte Bauteilartikel bleiben ausgewiesen.

## Teilumfang

- SCT013Head 0.2: korrigierter Lumberg-Anschluss dokumentiert; Abbildungen und Nullmessungsprotokolle zeigen noch Rev. 0.1. Die Firmware-Einrichtung ist auf der Sensorseite beschrieben.
- I²C Carrier HalfSize 0.1: Montage und Jumper beschrieben; vollständiger Bring-up fehlt.

## Noch zu erstellen

- SX1262-Remote: eigenständige Bestückungs-/Bring-up-Anleitung und eindeutiger Revisionsbezug.
- SolarRadiation: eigenständige Aufbauanleitung.
- RemoteNode: Revisions-/Dokumentationsumfang und Aufbauanleitung.

Die monolithische Test-Provisioning-Platine wurde durch die drei separaten Testboards abgelöst. Hierfür wird keine neue Aufbauanleitung geführt.

## Firmware-Releases

[Downloads und Änderungen](../firmware/downloads.md) enthalten veröffentlichte Firmwarestände mit Versionszuordnung, Commit und Prüfsumme. Firmwareversion und Hardware-Revision sind getrennte Angaben.

[Alte Revisionen](old-revisions.md)
