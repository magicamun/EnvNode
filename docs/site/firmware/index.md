# Firmware

Die gemeinsame ESP32-Firmware übernimmt Sensorabfrage, persistente Konfiguration,
WLAN, MQTT, Diagnose und Updates. Sensoren, Aktoren und lokale Controller können
passend zur angeschlossenen Hardware konfiguriert werden.

## Einstieg

1. Verwende einen zur Hardware passenden Firmwarestand und prüfe das Boardprofil.
2. Richte WLAN und die Gerätekonfiguration über die Weboberfläche ein.
3. Konfiguriere den angeschlossenen Sensor mit Bus und Adresse.
4. Prüfe Diagnose und Messwerte lokal.
5. Trage den MQTT-Broker und gegebenenfalls Zugangsdaten ein; prüfe danach die
   empfangenen Messwerte und den Gerätestatus.

Die Firmware unterstützt unter anderem OTA-Updates, Sensor-Home-Assistant-Discovery
und getrennte Sensor-, Aktor- und Controller-Konfiguration.

Konkrete Flash-Anleitung, Bildschirmbeispiele und MQTT-Beispieltopics werden in der
nächsten Ausbaustufe anhand eines ausgewählten Firmwarestands ergänzt.
