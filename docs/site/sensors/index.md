# Sensorhardware

Du suchst einen Sensor für eine bestimmte Aufgabe? Beginne mit
[Ich möchte messen](../applications/measurements.md). Hier stehen die technischen
Anschlüsse, unterstützten Implementierungen und eigenen Sensorbaugruppen im Vordergrund.

EnvNode verbindet Sensoren mit einer gemeinsamen Messwertverarbeitung.
Entscheidend sind die elektrische Schnittstelle und eine passende Firmware-Implementierung.
Eine erreichbare I²C-Adresse allein bedeutet noch keine Treiberunterstützung.

## Direkt unterstützte Sensoren

| Schnittstelle | Implementierung | Messgrößen |
| --- | --- | --- |
| [I²C](i2c.md) | BME280 | Temperatur, relative Feuchte, Luftdruck |
| [I²C](i2c.md) | SHT4x | Temperatur, relative Feuchte |
| [I²C](i2c.md) | SHTC3 | Temperatur, relative Feuchte |
| GPIO | AM2302 / DHT22 | Temperatur, relative Feuchte |
| GPIO-Interrupt | Rain Gauge / Kippwaage | Kippereignisse und Niederschlagszuwachs |
| GPIO-Frequenzeingang | RainDetector | Frequenz der kapazitiven Sensorfläche |

Außerdem sind Simulationen für Temperatur, Feuchte und Luftdruck verfügbar.
Die Liste folgt der Sensorregistry im dokumentierten Firmwarestand.

## Eigene Sensorhardware

- [RainDetectorController und Head](rain-detector/index.md): kapazitive Sensorfläche,
  Frequenzauswertung und Heizung; in der gemeinsamen Firmware integriert,
  Kalibrierung und Eignung des Aufbaus für den Einsatz prüfen.
- [SCT013 und SCT013Head](sct013/index.md): Stromwandler mit Spannungsaufbereitung;
  Effektivstrom in Ampere über einen analogen GPIO. Die
  [Aufbauanleitung Rev. 0.1](sct013-head/assembly.md) beschreibt Bestückung und
  Erstprüfung, auch ohne Klinkenbuchse und Sensor.
- [AnalogHydroPressure](../modules/analog-hydro-pressure/index.md): Modul für eine
  4–20-mA-Drucksonde; Hardwareanleitung und Status der Firmware-Anbindung.

[Sensoren in der Weboberfläche einrichten](../firmware/sensors.md)
