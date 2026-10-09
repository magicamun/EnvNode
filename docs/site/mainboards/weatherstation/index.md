# EnvNode Weather / Weatherstation

EnvNode Weather ist das Mainboard für eine fest installierte Wetterstation.
Es kombiniert ein ESP32-DevKitC mit einem Modulslot, zwei I²C-Bussen und
zusätzlichen direkten Sensoranschlüssen. Diese Seite beschreibt **Revision 0.2**.

[Bestückungs- und Bring-up-Anleitung herunterladen (PDF, Rev. 0.2)](../../assets/downloads/EnvNode_Weather_Bestueckungs_und_Bringup_Rev0.2.pdf){ .md-button }

## Überblick

| Merkmal | Ausführung |
| --- | --- |
| Verarbeitung | ESP32-DevKitC |
| Erweiterungen | Ein Modulslot A |
| I²C | I2C0 und I2C1, jeweils vier vierpolige JST-SH-Anschlüsse |
| Direkte Eingänge | Vier ADC-Anschlüsse |
| Zusätzliche GPIOs | GPIO14, GPIO16, GPIO17, GPIO27, GPIO32 und GPIO4 |
| Board Identity | 24LC32-EEPROM U2 auf I2C0, Adresse `0x50` |
| Optionale Module Identity | Slot A, Adresse `0x52` |
| Versorgung | Aufgelötetes ORing Power U1 |
| Gehäusegeometrie | BOX4U 5U310700 |

## Stromversorgung

Die Versorgung erfolgt über das aufgelötete [ORing-Power-Modul](../oring-power/index.md).
Das Mainboard besitzt keinen eigenen USB- oder separaten 5-V-Eingangsstecker.
ORing Power bietet einen USB-C- und einen zweipoligen JST-PH-Eingang für 5 V.
USB-C am Versorgungsmodul überträgt keine Daten und dient nicht zum Programmieren.

!!! warning "Nur eine geeignete 5-V-Quelle anschließen"
    USB-C und JST-PH am ORing-Power-Modul teilen sich dieselbe 5-V-Leitung.
    Unabhängige Netzteile dürfen dort nicht gleichzeitig angeschlossen werden,
    sofern sie nicht ausdrücklich für direkten Parallelbetrieb geeignet sind.

## Board Identity und Abblockung

Das Identity-EEPROM wird aus `+3V3_SYS` versorgt. C1 (100 nF) liegt zwischen
`+5V` und GND und blockt die 5-V-Versorgung ab. Das EEPROM besitzt keinen
separaten lokalen Abblockkondensator. Diese Ausführung ist bei EnvNode Mini
und EnvNode Weather identisch.

## I²C-Anschlüsse

Jeder Bus ist an vier vierpoligen JST-SH-Anschlüssen verfügbar:

| Pin | Signal |
| --- | --- |
| 1 | GND |
| 2 | `+3V3_SYS` |
| 3 | SDA |
| 4 | SCL |

Die 4,7-kΩ-Pull-ups sind über normalerweise offene Lötjumper einzeln zuschaltbar.
Prüfe vorhandene Pull-ups auf den Sensorboards und die Belegung aller Kabel.
Auf I2C0 sind `0x50` für die Board Identity und `0x52` für die optionale Slot-A-Identity vorgesehen.

[Sensoranbindung und Firmware-Konfiguration](../../sensors/i2c.md)

## Modulslot A

| Slot-Pin | Signal | Zuordnung |
| --- | --- | --- |
| 5 | I2C0 SDA | Datenleitung |
| 6 | I2C0 SCL | Taktleitung |
| 9 | AUX_GPIO1 | GPIO33 / ADC1_CH5 |
| 10 | AUX_GPIO2 | GPIO13 |

AUX_GPIO1 unterstützt digitale Ein- und Ausgänge/PWM sowie analoge Messung bei aktivem WLAN.
AUX_GPIO2 unterstützt digitale Ein- und Ausgänge/PWM; seine ADC2-Funktion ist während
WLAN-Betrieb nicht verfügbar. GPIO33 ist dem Modulslot zugeordnet und nicht einem
zusätzlichen JST-PH-Anschluss.

Eine erkannte Module Identity aktiviert keinen Treiber automatisch.
Die Sensor- und Aktorkonfiguration muss zur tatsächlichen Verdrahtung passen.

## Direkte ADC- und GPIO-Anschlüsse

Zehn vierpolige JST-PH-Anschlüsse stehen für direkte Signale bereit:

| Anschluss | ESP32-Signal |
| --- | --- |
| ADC1 | GPIO34 |
| ADC2 | GPIO35 |
| ADC3 | GPIO36 |
| ADC4 | GPIO39 |
| GPIO14 | GPIO14 |
| GPIO16 | GPIO16 |
| GPIO17 | GPIO17 |
| GPIO27 | GPIO27 |
| GPIO32 | GPIO32 |
| GPIO4 | GPIO4 |

Alle verwenden dieselbe Pinfolge:

| Pin | Signal |
| --- | --- |
| 1 | GND |
| 2 | `+5V` |
| 3 | Eingangssignal beziehungsweise GPIO |
| 4 | `+3V3_SYS` |

!!! warning "Versorgung und Signalpegel unterscheiden"
    Die 5-V-Leitung dient der Sensorversorgung. ESP32-Signale sind nicht 5-V-tolerant.
    GPIO34, GPIO35, GPIO36 und GPIO39 sind ausschließlich Eingänge und besitzen keine
    internen Pull-up- oder Pull-down-Widerstände. Konfiguriere sie nicht als Ausgänge.

## Mechanischer Aufbau

Platinenkontur und Gehäusebohrungen folgen der BOX4U-5U310700-Geometrie.
H1 und H2 sind M2,5-Stützbohrungen für das Modul; H3 bis H6 sind die
4,5-mm-Gehäusebefestigungsbohrungen. Prüfe Steckerzugang, Abstandshalter und
Kabelführung vor dem Einbau.

## Bestückung und erste Inbetriebnahme

Die [PDF-Anleitung](../../assets/downloads/EnvNode_Weather_Bestueckungs_und_Bringup_Rev0.2.pdf)
enthält den detaillierten Bestückungs- und Prüfablauf.

1. Platinenrevision und Anleitung abgleichen.
2. Mainboard und ORing Power bestücken; Lötstellen, Polarität und Durchgang nach Anleitung prüfen.
3. Versorgung zunächst ohne zusätzliche Sensoren oder Modul kontrollieren.
4. Passende Firmware verwenden und Boardprofil beziehungsweise Board Identity prüfen.
5. Einen unterstützten I²C-Sensor bei abgeschalteter Versorgung anschließen.
6. Bus, Adresse und Implementierung konfigurieren; Diagnose und Messwerte prüfen.
7. MQTT einrichten und weitere Anschlüsse einzeln in Betrieb nehmen.

[Zur Firmware-Übersicht](../../firmware/index.md)

## Prüfstand

Die Quellunterlagen dokumentieren am 28. September 2026 eine ERC-Prüfung ohne Befunde,
keine unverbundenen Pads und keine Schaltplan/PCB-Abweichungen nach Zonenfüllung.
Drei dokumentierte DRC-Ausnahmen betreffen zwei Courtyard-Überlappungen unter dem
DevKit und eine Siebdruckwarnung am Versorgungsmodul.

Diese historischen Konstruktionsprüfungen sind keine neue physische Validierung.
Die vorhandenen Unterlagen nennen noch ausstehende Inbetriebnahme und
Produktionsvalidierung; elektrische, thermische, EMV-, Umwelt- und Fertigungsprüfungen
sind damit nicht nachgewiesen.
