# RainDetectorController und Head

Der RainDetector besteht aus einer kapazitiven Sensorfläche mit Heizwiderständen
und einem Controller für Frequenzsignal und Heizschalter. Er erkennt Benetzung;
eine Niederschlagsmenge in Millimetern liefert eine separate Kippwaage.

## Was möchtest du tun?

| Dein Ausgangspunkt | Hier geht es weiter |
| --- | --- |
| Benetzung beobachten und Zustände verstehen | [Anwendung: Benetzung beobachten](../../applications/rain-detector.md) |
| Controller selbst aufbauen und prüfen | [Controller 0.4: Aufbau und elektrische Prüfung](../rain-detector-controller/assembly.md) |
| Sensorfläche selbst aufbauen und prüfen | [Head 0.2: Aufbau und elektrische Prüfung](../rain-detector-head/assembly.md) |
| Geprüfte Hardware mit EnvNode einrichten | [RainDetector in Betrieb nehmen](../../commissioning/rain-detector.md) |

## Zwei Baugruppen

| Baugruppe | Aufgabe |
| --- | --- |
| RainDetectorController 0.4 | Frequenzfront-end und Schalter für die Heizung |
| RainDetectorHead 0.2 | Kapazitive Sensorfläche und Heizwiderstände |

![Controller 0.4](../../assets/images/rain-detector/RainDetectorController-Rev0.4-top.png)

![Head 0.2](../../assets/images/rain-detector/RainDetectorHead-Rev0.2-top.png)

Anschlussbelegungen, Heizlast, Beschichtung und mechanische Prüfungen sind in den
jeweiligen Aufbauanleitungen dokumentiert. Die Laborerprobung der Firmware ist
von Feldtests und der Abnahme des realen Aufbaus zu unterscheiden.

[Kompatibilität](../../compatibility.md) ·
[Controller: alte Revisionen](../rain-detector-controller/old-revisions.md) ·
[Head: alte Revisionen](../rain-detector-head/old-revisions.md)
