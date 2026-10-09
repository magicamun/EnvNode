# EnvNode Mini

EnvNode Mini ist das kompakte ESP32-Mainboard der EnvNode-Plattform.
Es bietet zwei Modulslots, zwei I²C-Busse und direkte Eingänge für analoge oder digitale Signale.
Diese Seite beschreibt **Hardware-Revision 0.8**.

[Bestückungs- und Bring-up-Anleitung herunterladen (PDF, Rev. 0.8)](../../assets/downloads/EnvNode_Mini_Bestueckungs_und_Bringup_Rev0.8.pdf){ .md-button }

## Überblick

| Merkmal | Revision 0.8 |
| --- | --- |
| Verarbeitung | ESP32 |
| Erweiterungen | Zwei Modulslots A und B |
| I²C | I2C0 und I2C1, jeweils zwei vierpolige JST-SH-Anschlüsse |
| Direkte Eingänge | ADC1 an GPIO34 und ADC2 an GPIO35 |
| Board Identity | 24LC32-EEPROM auf I2C0, Adresse `0x50` |
| Optionale Module Identity | Slot A: `0x52`; Slot B: `0x53` |
| Versorgung | Aufgelötetes ORing-Power-Modul |

## Stromversorgung

Die Eingangs- und Versorgungsschaltung sitzt auf dem
aufgelöteten [ORing-Power-Modul](../oring-power/index.md). Das Mini-Mainboard selbst besitzt keinen eigenen
USB- oder separaten 5-V-Eingangsstecker.

ORing Power nimmt 5 V über USB-C oder den zweipoligen JST-PH-Eingang entgegen.
Der Regler erzeugt 3,3 V; Schottky-Dioden führen diese Quelle und die
3,3-V-Versorgung des ESP32-DevKit zur Systemversorgung zusammen.
Der USB-C-Anschluss von ORing Power dient ausschließlich der Stromversorgung,
nicht der Datenübertragung oder Programmierung.

!!! warning "5-V-Eingänge teilen sich eine Leitung"
    USB-C und JST-PH am ORing-Power-Modul sind nicht voneinander entkoppelt.
    Schließe keine unabhängigen Netzteile gleichzeitig an diese beiden Eingänge an,
    sofern sie nicht ausdrücklich für direkten Parallelbetrieb geeignet sind.

## Board Identity und Abblockung

Das Identity-EEPROM wird aus `+3V3_SYS` versorgt. C1 (100 nF) liegt zwischen
`+5V` und GND und blockt die 5-V-Versorgung ab. Das EEPROM besitzt keinen
separaten lokalen Abblockkondensator. Diese Ausführung ist bei EnvNode Mini
und EnvNode Weather identisch.

## I²C-Anschlüsse

Jeder Bus besitzt zwei vierpolige JST-SH-Anschlüsse mit dieser Pinbelegung:

| Pin | Signal |
| --- | --- |
| 1 | GND |
| 2 | `+3V3_SYS` |
| 3 | SDA |
| 4 | SCL |

Pro Bus gibt es ein Paar 4,7-kΩ-Pull-ups. Sie sind normalerweise über offene
Lötjumper getrennt. Prüfe vor dem Zuschalten, ob angeschlossene Sensorboards
bereits Pull-ups enthalten. Steckerbelegung und Busadresse müssen ebenfalls stimmen.

[Hinweise zur Sensoranbindung](../../sensors/i2c.md)

## Modulslots

Beide Slots folgen der gemeinsamen EnvNode-Modulschnittstelle.
I2C0 liegt an Slot-Pin 5 (SDA) und Pin 6 (SCL).

| Ressource | Slot A | Slot B |
| --- | --- | --- |
| Pin 9 / AUX_GPIO1 | GPIO33 / ADC1_CH5 | GPIO32 / ADC1_CH4 |
| Pin 10 / AUX_GPIO2 | GPIO13 | GPIO14 |
| Pin 14 / SPI_CS | GPIO5 | GPIO27 |
| EEPROM-Adresse | `0x52` | `0x53` |

AUX_GPIO1 ist als digitaler Eingang, Ausgang/PWM oder analoger Eingang auch
bei aktivem WLAN nutzbar. AUX_GPIO2 unterstützt digitale Ein- und Ausgänge/PWM;
seine ADC2-Funktion steht während des WLAN-Betriebs nicht zur Verfügung.

Die Firmware kann unterstützte Module identifizieren und ihre Kompatibilität
prüfen. Die Identifikation aktiviert keinen modulspezifischen Treiber automatisch.
Konfiguriere die tatsächlich angeschlossenen Sensoren und Aktoren ausdrücklich.

## Direkte ADC-Eingänge

Zwei vierpolige JST-PH-Anschlüsse stehen für direkte Eingangssignale bereit:

| Anschluss | Signal an Pin 3 | ESP32-ADC-Kanal |
| --- | --- | --- |
| ADC1 | GPIO34 | ADC1_CH6 |
| ADC2 | GPIO35 | ADC1_CH7 |

Beide Anschlüsse verwenden dieselbe Pinfolge:

| Pin | Signal |
| --- | --- |
| 1 | GND |
| 2 | `+5V` |
| 3 | Eingangssignal |
| 4 | `+3V3_SYS` |

!!! warning "Eingänge sind nicht 5-V-tolerant"
    GPIO34 und GPIO35 sind ausschließlich Eingänge und besitzen keine internen
    Pull-up- oder Pull-down-Widerstände. Das Signal an Pin 3 muss innerhalb des
    ESP32-3,3-V-Eingangsbereichs bleiben. Die 5-V-Leitung dient nur der Sensorversorgung.
    Diese Pins dürfen nicht als Ausgänge konfiguriert werden.

## Bestückung und erste Inbetriebnahme

Die [PDF-Anleitung für Revision 0.8](../../assets/downloads/EnvNode_Mini_Bestueckungs_und_Bringup_Rev0.8.pdf)
ist die maßgebliche Anleitung für Bestückung und elektrische Prüfungen.

1. Gleiche die Revision deiner Platine mit der Anleitung ab.
2. Bestücke und prüfe Mainboard und ORing Power nach den dort angegebenen Schritten.
3. Prüfe Kurzschlüsse, Polarität und Versorgung zunächst ohne zusätzliche Module und Sensoren.
4. Verwende passende Firmware und prüfe die angezeigte Board Identity beziehungsweise das Boardprofil.
5. Schließe bei abgeschalteter Versorgung einen unterstützten I²C-Sensor an.
6. Konfiguriere Bus, Adresse und Sensorimplementierung; prüfe die lokale Diagnose und Messwerte.
7. Richte MQTT ein und kontrolliere die empfangenen Messwerte im Zielsystem.

[Zur Firmware-Übersicht](../../firmware/index.md)

## Prüfstand

Die vorhandenen Entwicklungsunterlagen berichten für den 29. September 2026
über eine fehlerfreie ERC-Prüfung und keine aktiven DRC-Verstöße oder unverbundenen Pads
nach Zonenfüllung. Schaltplan und PCB waren konsistent; 18 dokumentierte Ausnahmen
betrafen Courtyard- und Siebdruckinteraktionen.

Diese Konstruktionsprüfungen ersetzen keine elektrischen, thermischen, EMV-, ESD-
oder Fertigungsprüfungen. Die Quellunterlagen nennen für Revision 0.8 noch ausstehende
physische Inbetriebnahme und Produktionsvalidierung. Die Anleitung beschreibt den
Aufbau und die Prüfungen; sie ist keine pauschale Produktionsfreigabe.
