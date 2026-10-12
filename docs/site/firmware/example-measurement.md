# Beispiel: Mini mit SHT4x bis zur MQTT-Messung

Ziel: Temperatur und relative Feuchte lokal sehen und über MQTT empfangen.
Voraussetzung sind ein geprüftes Mini-Mainboard, passende Firmware und ein
3,3-V-kompatibles SHT4x-I²C-Sensorboard mit Adresse 0x44.

## Anschließen

1. Versorgung ausschalten.
2. Sensor an einen I2C0-Anschluss verbinden: GND, 3,3 V, SDA, SCL korrekt zuordnen.
3. Pull-ups des Sensorboards und die Mainboard-Jumper abgleichen.
4. Einschalten und Weboberfläche öffnen.

## Einrichten und lokal prüfen

1. Unter Hardware das aktive Mini-Profil prüfen.
2. Unter Diagnose I2C0 scannen: 0x44 muss erreichbar sein.
3. Unter Sensoren einen freien Platz als SHT4x, Bus I2C0, Adresse 0x44 einrichten.
4. Einen Namen wie „Umgebung“ vergeben, speichern und gegebenenfalls anwenden.
5. Unter Messwerte Temperatur und relative Feuchte des richtigen Sensorplatzes prüfen.

Erwartung: Physische Werte werden aktualisiert, der Sensor meldet keinen
unerklärten Fehler. Ein erfolgreicher Scan ohne Messungen genügt nicht.

## MQTT anschließen

1. Broker unter MQTT eintragen und Verbindung prüfen.
2. Mit einem MQTT-Client `envnode/<device>/sensor/#` abonnieren.
3. Quelle und Werte mit der lokalen Anzeige vergleichen.
4. Bei Home Assistant die MQTT-Integration voraussetzen und Discovery prüfen.

`<device>` durch den verwendeten Topic-Gerätenamen ersetzen.
Keine erfundenen Beispielmesswerte als reale Testergebnisse übernehmen.

## Abschluss

Neustart durchführen und erneute Verbindung sowie Messung prüfen.
[Konfiguration sichern](maintenance.md), wenn der Aufbau funktioniert.
