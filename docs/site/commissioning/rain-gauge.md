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

## Millimeter pro Kippung und Entprellung

| Feld | Einheit | Wirkung |
| --- | --- | --- |
| Millimeter pro Kippung | mm pro angenommenem Ereignis | Niederschlagszuwachs, der für eine Kippung veröffentlicht wird; nicht die bereits gefallene Regenmenge. |
| Entprellzeit | ms | Nach einer angenommenen Kippung werden weitere Flanken innerhalb dieser Zeit verworfen. Zu kurz kann mehrfach zählen, zu lang echte Kippungen verlieren. |

Die Millimeter pro Kippung zunächst aus den Unterlagen der konkreten Wippe
übernehmen. Der zulässige Eingabebereich der Firmware ist 0,0001 bis 100 mm;
dieser Bereich ist keine Empfehlung für einen bestimmten Sensor. Die Entprellzeit
kann von 1 bis 5000 ms eingestellt werden und muss zum Kontakt passen.

## Regenwippe kalibrieren

Zuerst die Erfassung bekannter Einzelkippungen prüfen. Ein falscher Skalierungsfaktor
kann verlorene oder doppelt gezählte Ereignisse nicht beheben. Die Wippe gemäß
Herstellerangabe ausrichten und das dort beschriebene Kalibrierverfahren verwenden.

Für ein Verfahren mit bekanntem aufgefangenem Wasservolumen gilt rechnerisch:

```text
Millimeter pro Kippung = 10 × Wasservolumen in ml / (Auffangfläche in cm² × Kippungen)
```

Dabei ist die Auffangfläche die wirksame horizontale Öffnung des Trichters, nicht
die Fläche der Wippenschale. Der Zusammenhang gilt nur, wenn das bekannte Volumen
tatsächlich vollständig zur erfassten Kippzahl gehört. Langsam und gleichmäßig
zuführen; Restwasser, teilweise gefüllte Wippenschalen und Verluste beeinflussen
sonst das Ergebnis. Mehrere Durchläufe vergleichen, bevor ein Wert übernommen wird.

Den ermittelten Faktor speichern, Sensoränderungen anwenden und einen unabhängigen
Kontrolldurchlauf durchführen. Geänderte Faktoren wirken auf **zukünftige**
Kippereignisse; bereits veröffentlichte Niederschlagszuwächse werden nicht
rückwirkend korrigiert.

## Mit bekannten Kippungen prüfen

Die Wippe zunächst trocken und kontrolliert mehrfach einzeln betätigen.
Prüfe Kippereignisse und Niederschlagszuwachs. Bei N Kippungen muss der Zuwachs
N × Millimeter pro Kippung entsprechen. Doppelzählungen deuten auf Prellen oder
Störungen hin; fehlende Ereignisse unter anderem auf Anschluss oder zu lange
Entprellzeit. Danach den Betrieb mit Wasser gemäß Herstellerverfahren prüfen.

## Ereignisse und Summen unterscheiden

Für jede angenommene Kippung entstehen ein Kippereignis (`rain_gauge_tip`) und
ein Niederschlagszuwachs (`rainfall_increment`) in der eingestellten Größe.
Der Zahlenwert des einzelnen Zuwachses bleibt bei gleichbleibender Kalibrierung
also gleich. Die Messwertseite zeigt keine fortlaufende Regen-Gesamtsumme und
ersetzt kein Ereignisprotokoll. Für den Zähltest die Ereignisse beispielsweise
über [MQTT](../firmware/mqtt.md) mitverfolgen; N Ereignisse ergeben N mal den
konfigurierten Zuwachs.

Nach einem Neustart Einstellungen und erneute Erfassung kontrollieren und die
[Konfiguration sichern](../firmware/maintenance.md). Tages- und Langzeitsummen
bildet das empfangende System aus den Messungen; sie sind keine lokale Wetterauswertung.

[MQTT und Home Assistant](../firmware/mqtt.md) · [Messaufgaben](../applications/measurements.md)
