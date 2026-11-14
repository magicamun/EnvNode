# AnalogHydroPressure

AnalogHydroPressure verbindet eine 4–20-mA-Drucksonde mit einem ADS1115 und stellt
über einen lokalen Boost-Wandler die Sondenversorgung bereit. Damit lassen sich
Druck und – mit passender Sonde und Umrechnung – Füllstand erfassen.

## Was möchtest du tun?

| Dein Ausgangspunkt | Hier geht es weiter |
| --- | --- |
| Modul selbst aufbauen oder elektrisch prüfen | [Aufbau und elektrische Prüfung, Revision 0.6](assembly.md) |
| Bus, Adresse oder Sondenanschluss nachsehen | [Anschlüsse und Konfiguration](assembly.md#4-anschlusse-und-konfiguration) |
| Das Modul in der EnvNode-Firmware einrichten | Siehe Firmwareunterstützung unten |
| Eine andere Messaufgabe umsetzen | [Unterstützte Sensoren auswählen](../../applications/measurements.md) |

## Firmwareunterstützung

Für diesen Messpfad gibt es derzeit keine fertige Sensorauswahl in der gemeinsamen
EnvNode-Firmware. Eine ADS1115-Adresse im I²C-Scan oder ein gültiger Modul-Descriptor
bestätigt noch keine Druckmessung und aktiviert nicht automatisch den Boost.
Eine Software-Inbetriebnahmeanleitung folgt mit der integrierten Unterstützung.

Buswahl und Adressjumper sind in der technischen Anleitung revisionsbezogen
beschrieben. Die Messkette muss mit einer geeigneten Stromquelle und anschließend
mit der konkreten Sonde geprüft werden.

[Kompatibilität und Firmwarezuordnung](../../compatibility.md) · [Alte Revisionen](old-revisions.md)
