# Grundlagen

## Vom Sensor zum Messwert

Ein Sensor erfasst eine physikalische Größe. Die Firmware liest ihn aus,
führt die für die Messung erforderliche Umrechnung oder Kalibrierung durch und
veröffentlicht den Messwert über MQTT. Externe Systeme übernehmen Speicherung,
Visualisierung und weitergehende Interpretation.

## Die Bausteine

| Baustein | Aufgabe |
| --- | --- |
| Mainboard | ESP32, Stromversorgung und Schnittstellen bereitstellen |
| ORing Power | 5-V-Eingang, 3,3-V-Regelung und Zusammenführung der 3,3-V-Quellen |
| Erweiterungsmodul | Zusätzliche Sensor- oder Aktoranschlüsse bereitstellen |
| Sensor | Physikalische Messwerte liefern |
| Firmware | Konfiguration, Messbetrieb, lokale Steuerung und Kommunikation |

Sensoren, Aktoren und lokale Controller besitzen getrennte Aufgaben.
Lokale Controller können konfigurierte Aktoren steuern; Wetterinterpretation,
Langzeitauswertung und übergeordnete Automationen gehören in externe Systeme.

## Identifikation und Konfiguration

Board Identity und optionale Module Identity beschreiben die eingesetzte Hardware.
Die Erkennung eines Moduls bedeutet nicht automatisch, dass ein passender Treiber
aktiviert wird. Sensoren und Aktoren müssen passend zur tatsächlichen Verdrahtung
konfiguriert werden.
