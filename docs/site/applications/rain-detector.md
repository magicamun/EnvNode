# Benetzung beobachten

Mit RainDetectorController und Head kannst du Benetzung einer Sensorfläche
erkennen und die Fläche anschließend automatisch trocknen. Für Niederschlagsmengen
in Millimetern verwendest du eine [Regenwippe](../commissioning/rain-gauge.md).

## Dein Weg zur Anwendung

| Dein Ausgangspunkt | Hier geht es weiter |
| --- | --- |
| Hardware noch nicht aufgebaut | [Baugruppen und technische Anleitungen](../sensors/rain-detector/index.md) |
| Hardware geprüft, Software noch nicht eingerichtet | [RainDetector in Betrieb nehmen](../commissioning/rain-detector.md) |
| Automatik läuft bereits | Zustände unten prüfen und weiterverwenden |

## Was sagen die Zustände aus?

Die Frequenz ist der physische Messwert. Der lokale Controller liefert zusätzlich
Regenzustand, Gültigkeit, Heizanforderung, Trockenbestätigung und Anzahl erkannter
Benetzungen. **Regenende geschätzt** bedeutet eine Ruhezeit ohne neue Benetzung;
es sagt noch nicht, dass die Fläche trocken ist. **Heizung an** beschreibt die
Anforderung und bestätigt keine reale Heizleistung.

Ungültige Messung ist weder trocken noch regenfrei. Bewerte die Gültigkeit zusammen
mit dem Zustand. Die Schwellen müssen zum eigenen Head passen; mechanische
Abdichtung und Beschichtung sind Teil der Eignung für den Außeneinsatz.

## Zustände anzeigen oder weitergeben

[Displays und Values](../firmware/values-displays.md) können Sensor- und
Controller-Eigenschaften anzeigen. Für weitere Systeme beschreibt
[MQTT und Home Assistant](../firmware/mqtt.md) Verbindung und Topics.
Langzeitaufzeichnung und Wetterauswertung erfolgen im empfangenden System.
