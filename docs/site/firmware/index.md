# Firmware

Die ESP32-Firmware erfasst Messwerte, stellt Konfiguration und Diagnose bereit
und verbindet EnvNode über MQTT mit externen Systemen.
Sie trennt Sensoren, Aktoren, lokale Controller und Anzeige.

## Einstieg

1. [Firmware bauen und aufspielen](installation.md).
2. [Netzwerk, Sensoren und Aktoren konfigurieren](configuration.md).
3. [MQTT und externe Integration prüfen](mqtt.md).
4. [Updates und Fehlerdiagnose](maintenance.md).

## Funktionsumfang

| Bereich | Vorhandene Funktionen |
| --- | --- |
| Netzwerk | WLAN, Setup-Access-Point, Wiederverbindung, Hostname |
| Konfiguration | Persistente Speicherung im ESP32-NVS |
| Messung | Physische und simulierte Sensoren, typisierte Messwerte |
| Aktoren | GPIO On/Off und PWM/Level |
| Controller | Lokale Steuerung, darunter Blink, Threshold und Selector |
| Anzeige | Zwei I²C-Displayausgänge und konfigurierbare Textseiten |
| Integration | MQTT, Sensor-Home-Assistant-Discovery |
| Wartung | Diagnose, Logs, Web-OTA und Factory Reset |

Die [Sensorübersicht](../sensors/index.md) nennt die tatsächlich registrierten
Treiber. Entwicklungs-Hardware ist nicht automatisch als Sensor integriert.
Die Firmware übernimmt keine Wettervorhersage, langfristige Historie oder ETo-Berechnung.

## Stand und Reproduzierbarkeit

Diese Anleitung folgt dem Quellstand vom 9. Oktober 2026.
Die Weboberfläche **Firmware** zeigt Version, Build-Identität, Git-Stand und
Quellzustand des laufenden Geräts. Diese Angaben für Tests und Fehlerberichte notieren.
Ein verfügbarer Treiber ist kein Nachweis einer vollständigen Hardwarequalifikation.
