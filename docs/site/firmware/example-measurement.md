# Beispiel: Mini mit SHT4x bis zur MQTT-Messung

Ziel: Temperatur und relative Feuchte lokal sehen und über MQTT empfangen.
Du benötigst ein geprüftes Mini-Mainboard und ein geeignetes 3,3-V-SHT4x-Sensorboard.

## Sensor zuerst in Betrieb nehmen

Die [I²C-Inbetriebnahme](../commissioning/i2c-sensors.md) ist die maßgebliche
Anleitung für Anschluss, Einrichtung und lokale Messwertprüfung.
Für dieses Beispiel verwende I2C0, SHT4x an 0x44 und den Namen „Umgebung“.
Erst mit aktuellen, gültigen lokalen Werten folgt die externe Anbindung.

## MQTT anschließen

Die [MQTT-Anleitung](mqtt.md) beschreibt Brokerverbindung und Discovery.
Mit einem MQTT-Client `envnode/<device>/sensor/#` abonnieren und die Quelle
sowie Werte mit der lokalen Anzeige vergleichen. `<device>` durch den
tatsächlichen Topic-Gerätenamen ersetzen.

Für Home Assistant müssen MQTT-Integration und Discovery eingerichtet sein.
Die extern angezeigte Messung muss zur lokalen Quelle gehören und bei weiteren
Messungen aktualisiert werden. Fehlende Daten sind kein Wert von null.

Nach Neustart erneute Verbindung und Messung prüfen und
[Konfiguration sichern](maintenance.md).
