# Sensoren

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

Außerdem sind Simulationen für Temperatur, Feuchte und Luftdruck verfügbar.
Die Liste folgt der Sensorregistry im dokumentierten Firmwarestand.

## Eigene Sensorhardware

- [RainDetectorController und Head](rain-detector.md): kapazitive Sensorfläche,
  Frequenzauswertung und Heizung; Entwicklungsstand mit separater Laborfirmware.
- [SCT013 und SCT013Head](sct013.md): Stromwandler mit Spannungsaufbereitung;
  Entwicklungsstand ohne integrierten Firmware-Treiber.
- [AnalogHydroPressure](../modules/analog-hydro-pressure/index.md): Modul für eine
  4–20-mA-Drucksonde; Hardwareanleitung und Status der Firmware-Anbindung.

[Sensoren konfigurieren](../firmware/configuration.md)
