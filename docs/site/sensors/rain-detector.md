# RainDetectorController und Head

!!! note "Entwicklungsstand / WIP"
    Diese Seite beschreibt die vorhandene Hardware und ihre Prüfgrenzen.
    RainDetector ist noch kein registrierter Sensor der gemeinsamen EnvNode-Firmware.
    Eine separate Laborfirmware misst die Ausgangsfrequenz; sie steuert keine Heizung.

## Zwei getrennte Baugruppen

| Baugruppe | Revision | Aufgabe |
| --- | --- | --- |
| RainDetectorController | 0.3 | ICM7555-Frequenzfront-end und AO3400A-Heizschalter |
| RainDetectorHead | 0.1 | Passive kapazitive Sensorfläche und Heizwiderstände |

Die Sensorfläche besitzt zwei von Lötstopplack bedeckte, ineinandergreifende
Kupferelektroden. Der Controller wandelt das Sensorsignal in einen digitalen
3,3-V-Frequenzausgang um. Eine Frequenzänderung ist noch kein kalibrierter
Niederschlagswert und ersetzt keine Kippwaagenmessung.

## Verbindungen zum Mainboard

| Anschluss | Pin 1 | Pin 2 | Pin 3 | Pin 4 |
| --- | --- | --- | --- | --- |
| J3 | GND | +5V | HEATER_EN | +3V3 |
| J4 | GND | +5V | RAIN_FREQ | +3V3 |

Die Versorgungspins sind gemeinsam geführt. J3/J4 sind in der vorhandenen
Bestückungsbasis DNP; Anschlussvariante und passende Stecker müssen für den
konkreten Aufbau ausgewählt werden. HEATER_EN benötigt einen geeigneten
Digitalausgang, RAIN_FREQ einen geeigneten Frequenzmesseingang.

## Verbindung zwischen Head und Controller

| Head-Kontakt | Signal | Controller-Kontakt |
| --- | --- | --- |
| J1.1 | HEATER+ | J1.1 |
| J1.4 | GND / Elektrode 2 | J1.4 |
| J2.1 | SENSE / Elektrode 1 | J2.1 |
| J2.4 | HEATER− | J2.4 |
| Pins 2 und 3 | Mechanische Stützung, elektrisch unverbunden | Entsprechend unverbunden |

Die Unterseiten beider Platinen zeigen zueinander. Die 2×2-Steckverbindungen
müssen elektrisch und mechanisch korrekt ausgerichtet sein; die Tabelle allein
bestätigt weder die Kontaktlage noch die reale Steckhöhe.

## Heizung und Fertigung

Zwanzig 15-Ω-Widerstände bilden fünf parallele Stränge mit jeweils vier
Widerständen. Das ergibt nominal 12 Ω, etwa 417 mA und 2,08 W an 5 V.
Versorgung und Kabel müssen diese Last zusätzlich zu den übrigen Systemlasten tragen.
Die Anleitung sieht Widerstände mit mindestens 0,25 W und Beachtung der
Temperaturabhängigkeit ihrer Belastbarkeit vor.

C1 des Heads bezeichnet die gefertigte Kupfer-Sensorstruktur, kein aufzulötendes Bauteil.
Via-Verschluss, Beschichtung und Wasserausschluss müssen mit dem Fertiger abgestimmt
und am aufgebauten Head geprüft werden. Lötstopplack allein ist keine Zusage der Dichtheit.

## Getrennte Inbetriebnahme

1. Beide Platinen spannungsfrei auf richtige Kontaktzuordnung und Kurzschlüsse prüfen.
2. Controller-Versorgung und 3,3-V-Frequenzausgang separat prüfen.
3. Frequenz mit trockenem und benetztem Head vergleichen und mit einem Messgerät kontrollieren.
4. Heizpfad separat prüfen: Stromaufnahme, Erwärmung, Abschaltung und Temperaturverhalten.
5. Erst danach den vollständigen mechanischen Aufbau und Wasserausschluss qualifizieren.

## Firmwarestand

Das separate Projekt `lab/RainDetector-PCNT` zählt steigende Flanken über ESP32-PCNT
und gibt Frequenzwerte über Serial aus. GPIO27 ist dessen Laborvorgabe, keine feste
Zuordnung für alle EnvNode-Mainboards. Es gibt dort kein MQTT und keine Heizautomation.
Das Aufspielen ersetzt die bisherige Firmware auf dem Testgerät.

Die gemeinsame EnvNode-Firmware enthält einen Rain-Gauge-Treiber für Kippwaagen.
Dieser ist funktional verschieden und kein RainDetector-Treiber.

## Anleitungen und Prüfstand

- [Controller: Bestückung und Bring-up (PDF)](../assets/downloads/RainDetectorController_Bestueckungs_und_Bringup_Rev0.3.pdf)
- [Head: Bestückung und Bring-up (PDF)](../assets/downloads/RainDetectorHead_Bestueckungs_und_Bringup_Rev0.1.pdf)

Die dokumentierten ERC/DRC-Prüfungen vom 14. September 2026 liegen vor den
anschließenden Stecker- und Konturänderungen. Vor Fertigung sind aktuelle Prüfungen
nötig. Steckhöhe, reale Heizwirkung, Beschichtung und Wasserausschluss sind noch offen.
