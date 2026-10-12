# Sensoren einrichten und Messwerte prüfen

## Sensorplatz vorbereiten

Unter **Sensoren (Sensors)** einen freien Platz bearbeiten, einen verständlichen
Namen vergeben und die passende Implementierung wählen. Für einen Test ohne
Hardware eine simulierte Implementierung verwenden und sie erkennbar benennen.
Simulation bestätigt den Softwareablauf, nicht die reale Verkabelung.

| Implementierung | Schnittstelle | Einstellungen |
| --- | --- | --- |
| BME280 | I²C | Bus, Adresse 0x76 oder 0x77 |
| SHT4x | I²C | Bus, Adresse 0x44 |
| SHTC3 | I²C | Bus, Adresse 0x70 |
| AM2302 / DHT22 | GPIO | Geeigneter Einleitungs-GPIO |
| Rain Gauge | GPIO-Interrupt | GPIO, Millimeter pro Kippung, Entprellung |

Die tatsächliche Adresse und Pegel müssen zum Sensorboard passen.
[Anschluss und Pull-ups](../sensors/i2c.md) vor dem Einschalten prüfen.
Ein I²C-Scan ist eine Hilfe bei der Verkabelungsprüfung, kein automatisches Einrichten.

## Speichern, anwenden, kontrollieren

1. Implementierung und Ressourcen passend zur Hardware setzen.
2. Konfiguration speichern.
3. Falls angezeigt, ausstehende Sensoränderungen anwenden.
4. Sensorzustand kontrollieren und eine Messperiode abwarten.
5. Unter **Messwerte (Measurements)** Quelle, Messgröße, Einheit und Zustand prüfen.

Ein fehlender Wert ist nicht automatisch null. Bei fehlenden, ungültigen oder
alten Werten erst Zeit, Sensorzustand und Logs untersuchen. Nicht nur den
angezeigten Zahlenwert beurteilen.

## Typische Fehler

| Symptom | Erste Prüfung |
| --- | --- |
| Adresse fehlt im Scan | Versorgung, Bus, SDA/SCL, Pull-ups |
| Adresse vorhanden, Sensor fehlerhaft | Implementierung, erlaubte Adresse, Sensorvariante |
| Gespeichert, aber keine Änderung | Ausstehende Änderungen angewendet? |
| Lokal Werte, extern keine | MQTT-Verbindung und Topics |

RainDetector ist kein Rain-Gauge-Treiber. SCT013 und ADS1115 sind im aktuellen
Sensorregistry nicht als fertige Auswahl enthalten.

[Durchgehendes SHT4x-Beispiel](example-measurement.md)
