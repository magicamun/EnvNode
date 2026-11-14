# SCT013 und SCT013Head

SCT013 mit integriertem Spannungsausgang und SCT013Head ermöglichen die Messung
von **Wechselstrom als Effektivwert in Ampere**. Die Firmware berechnet daraus
keine Leistung oder Energie. Varianten mit reinem Stromausgang sind durch diesen
Head nicht abgedeckt; Nennstrom und Ausgangsspannung müssen zum Aufbau passen.

Aktueller Head ist **Rev. 0.2**. Er korrigiert den falsch angeschlossenen
Lumberg-Signalkontakt der Rev. 0.1. Ein unveränderter Rev.-0.1-Head benötigt vor
Verwendung die in der technischen Anleitung beschriebene Anschlusskorrektur.

## Was möchtest du tun?

| Dein Ausgangspunkt | Hier geht es weiter |
| --- | --- |
| Head selbst aufbauen, Buchse und Anschluss prüfen | [Aufbau und elektrische Prüfung Rev. 0.2](../sct013-head/assembly.md) |
| Geprüften Head mit Firmware einrichten | [SCT013 in Betrieb nehmen](commissioning.md) |
| Kalibrieren, Nullschwellen oder Diagnose verstehen | [Einrichtung und Messwertprüfung](commissioning.md#sensor-konfigurieren) |
| Eine andere Messgröße erfassen | [Sensorwahl nach Anwendung](../../applications/measurements.md) |

## In der Firmware einrichten

Die gemeinsame Firmware unterstützt **SCT013 AC RMS** mit DMA-Erfassung ab
**0.8.14**. Genau ein SCT013-Slot kann gleichzeitig aktiv sein; RainDetector kann
parallel laufen. Die [Inbetriebnahmeanleitung](commissioning.md) enthält die
Schritte, Vergleichswerte des Versuchsaufbaus und Diagnoseinformationen.

Die vorhandenen Hardware-Abbildungen zeigen noch Rev. 0.1 und sind entsprechend
gekennzeichnet. Für Rev. 0.2 ist die PCB-Artikelzuordnung noch zu ergänzen.

[Kompatibilität und Mindestfirmware](../../compatibility.md)
