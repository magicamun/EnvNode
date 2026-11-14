# I²C-Sensoren in Betrieb nehmen

Ziel: Ein angeschlossener BME280, SHT4x oder SHTC3 liefert aktuelle, gültige Messwerte.
Voraussetzung sind ein elektrisch geprüftes EnvNode, passende Firmware und eine
erreichbare Weboberfläche. [Gerät zuerst einrichten](../firmware/configuration.md).

## Anschluss und Ressource prüfen

Sensor bei ausgeschalteter Versorgung anschließen. Versorgung, Kabelbelegung,
Pull-ups und belegte Adressen beschreibt die [I²C-Hardwareseite](../sensors/i2c.md).
Notiere den verwendeten Bus und die tatsächliche Sensoradresse.
Unter **Hardware** muss das aktive Boardprofil zum Mainboard passen.
Ein I²C-Scan auf dem gewählten Bus sollte die Sensoradresse zeigen; er richtet
noch keinen Sensor ein.

## Sensor einrichten

Unter **Sensoren (Sensors)** einen freien Platz wählen, benennen und aktivieren.

| Dein Sensor | Implementierung | Adresse |
| --- | --- | --- |
| BME280 | BME280 | 0x76 oder 0x77, passend zum Sensorboard |
| SHT4x | SHT4x | 0x44 |
| SHTC3 | SHTC3 | 0x70 |

Den tatsächlichen Bus auswählen, speichern und ausstehende Sensoränderungen anwenden.
Die [Firmware-Anleitung](../firmware/sensors.md) erklärt das Formular und zeigt
als Beispiel die BME280-Konfiguration.

## Ergebnis prüfen

Unter **Messwerte (Measurements)** die richtige Quelle, Einheiten, Gültigkeit
und wiederholte Aktualisierung prüfen. BME280 liefert zusätzlich Luftdruck.
Fehlt eine Adresse, prüfe zuerst Anschluss und Versorgung; ist sie vorhanden,
aber die Messung fehlerhaft, prüfe Implementierung, Adresse und Sensorvariante.

Nach einem Neustart müssen Konfiguration und Messung wieder verfügbar sein.
Anschließend [Konfiguration sichern](../firmware/maintenance.md).
Für die weitere Verwendung: [Anzeigen](../firmware/values-displays.md) oder
[MQTT und Home Assistant](../firmware/mqtt.md).
