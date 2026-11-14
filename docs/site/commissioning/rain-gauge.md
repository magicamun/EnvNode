# Regenwippe in Betrieb nehmen

Ziel: Der Sensor **Rain Gauge** erfasst Kippereignisse und den zugehörigen
Niederschlagszuwachs. Er benötigt einen passenden Kippkontakt und dessen
Herstellerangabe **Millimeter pro Kippung**. Eine RainDetector-Sensorfläche
ist ein anderer Sensortyp.

## Anschluss vorbereiten

Verwende einen im aktiven Boardprofil verfügbaren GPIO mit Interrupt-Eignung.
Die [GPIO-Schnittstelle](../interfaces/index.md) beschreibt die elektrische
Belegung. Der Treiber wertet fallende Flanken aus und aktiviert den internen
Pull-up. Ein potentialfreier Schließerkontakt wird zwischen Eingang und GND
angeschlossen. Für aktive Sensorausgänge müssen Signalpegel und Beschaltung
vorher zum ESP32-Eingang passen. Keine fremde Versorgung auf den GPIO legen.
Vor der Verdrahtung ausschalten.

## In der Firmware einrichten

Unter **Sensoren** einen freien Platz als **Rain Gauge** aktivieren und den
verdrahteten GPIO wählen. Die Millimeter pro Kippung aus der Angabe der konkreten
Wippe eintragen. Die Entprellzeit muss Kontaktprellen unterdrücken und zugleich
echte aufeinanderfolgende Kippungen zulassen. Speichern und Sensoränderungen
anwenden; die gemeinsamen Aktionen erklärt [Sensoren einrichten](../firmware/sensors.md).

## Mit bekannten Kippungen prüfen

Die Wippe zunächst trocken und kontrolliert mehrfach einzeln betätigen.
Prüfe Kippereignisse und Niederschlagszuwachs. Bei N Kippungen muss der Zuwachs
N × Millimeter pro Kippung entsprechen. Doppelzählungen deuten auf Prellen oder
Störungen hin; fehlende Ereignisse unter anderem auf Anschluss oder zu lange
Entprellzeit. Danach den Betrieb mit Wasser gemäß Herstellerverfahren prüfen.

Nach einem Neustart Einstellungen und erneute Erfassung kontrollieren und die
[Konfiguration sichern](../firmware/maintenance.md). Tages- und Langzeitsummen
bildet das empfangende System aus den Messungen; sie sind keine lokale Wetterauswertung.

[MQTT und Home Assistant](../firmware/mqtt.md) · [Messaufgaben](../applications/measurements.md)
