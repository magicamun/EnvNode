# EnvNode Weather 0.2.1

EnvNode Weather ist das Mainboard für eine fest installierte Wetterstation.
Es kombiniert ein ESP32-DevKitC mit einem Modulslot, zwei I²C-Bussen und
zusätzlichen direkten Sensoranschlüssen. Diese Seite beschreibt **PCB-Revision 0.2.1**.

[Bestückungs- und Bring-up-Anleitung herunterladen (PDF, PCB 0.2.1)](../../assets/downloads/EnvNode_Weather_Bestueckungs_und_Bringup_Rev0.2.1.pdf){ .md-button }

Dokumentstand: **18. Oktober 2026**, mit aktuellen Renderings des PCB-Stands **0.2.1**.
Die Board Identity bleibt bei **0.2**; bisherige Prüfprotokolle behalten ihren historischen Stand.

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


## Anschlussdetails als Referenz

Die folgenden Detailbilder zeigen die vierpoligen Anschlüsse **ADC1 (JST-PH,
2,00 mm)** und **J5 (JST-SH, 1,00 mm)** des EnvNode Mini. Sie wurden direkt aus
den PCB-Daten und den zugehörigen STEP-Modellen gerendert. Die Pinbelegung ist
gegen PCB und Schaltplan geprüft; Pin 1 ist jeweils orange als **GND** markiert.
Die kleinen PCB-Ausschnitte besitzen virtuelle Schnittkanten.

Die beiden konkreten Ausschnitte sind **Referenzabbildungen für alle entsprechenden
vierpoligen JST-PH- und JST-SH-Anschlüsse auf Mini und Weather**. Ihre Pinfolge ist
bei allen Anschlüssen des jeweiligen Typs gleich. Referenzname, Signal an PH-Pin 3,
I²C-Bus und Einbaurichtung richten sich nach dem jeweiligen Boardanschluss.
Die Bilder zeigen die Bestückungsseite, nicht die Kontaktansicht eines Kabelsteckers;
bei anders gedrehten Anschlüssen Pin 1 am tatsächlichen Footprint zuordnen.
Der zweipolige Versorgungseingang von ORing Power ist von dieser Referenz nicht erfasst.

### JST-PH: ADC- und GPIO-Anschlüsse

![JST-PH ADC1: Schrägansicht mit markiertem Pin 1](../../assets/images/connectors/ADC1-oblique-annotated.png){ width="48%" }
![JST-PH ADC1: Draufsicht mit originalem Silkscreen](../../assets/images/connectors/ADC1-top-annotated.png){ width="48%" }

Von Pin 1 bis 4: **GND, +5V, Signal, +3V3_SYS**. Im gezeigten Mini-Ausschnitt
läuft diese Folge von oben nach unten. Der Silkscreen `GPIO` bezeichnet hier das
ADC1-Signal an GPIO34; bei den weiteren Anschlüssen ist Pin 3 dem jeweils
angegebenen ADC- oder GPIO-Signal zugeordnet. Die 5-V-Leitung ist eine Versorgung,
kein zulässiger Signalpegel für den ESP32.

### JST-SH: I²C-Anschlüsse

![JST-SH J5: Schrägansicht mit markiertem Pin 1](../../assets/images/connectors/J5-oblique-annotated.png){ width="48%" }
![JST-SH J5: Draufsicht mit Referenz und originalem Silkscreen](../../assets/images/connectors/J5-top-annotated.png){ width="48%" }

Von Pin 1 bis 4: **GND, +3V3_SYS, SDA, SCL**. Im gezeigten Mini-Ausschnitt
läuft diese Folge von links nach rechts. J5 gehört zu I2C0; deshalb stehen dort
`SDA0` und `SCL0`. An I2C1 gilt dieselbe Pinfolge für SDA1 und SCL1.
Die seitlichen Metalllaschen sind mechanische Befestigungen und keine Signalpins.
Die Draufsicht zeigt die Referenz `J5`, die in der Schrägansicht vom Gehäuse verdeckt wird.

Bei Weather gilt die PH-Referenz für ADC1 bis ADC4 und alle sechs direkten
GPIO-Anschlüsse; die SH-Referenz gilt für I2C0_1 bis I2C0_4 und I2C1_1 bis I2C1_4.
Die abgebildeten Beschriftungen stammen vom Mini. Weather ergänzt den entsprechenden
Silkscreen mit Beschriftungsstand 0.2.1; Position und Einbaurichtung richten sich
weiterhin nach dem tatsächlichen Weather-Anschluss.

Die PDF-Anleitung enthält dieselben Anschlussreferenzen als Ergänzung vom
18. Oktober 2026. Hardware-Revision und historische Prüfstände bleiben unverändert.

## Bestückung und erste Inbetriebnahme

Die [PDF-Anleitung](../../assets/downloads/EnvNode_Weather_Bestueckungs_und_Bringup_Rev0.2.1.pdf)
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

## Mit der Firmware verwenden

[Weather 0.2 provisionieren](../../firmware/provisioning.md#weather-revision-02-provisionierung).


[Erste Inbetriebnahme](../../firmware/configuration.md),
[Board Identity](../../firmware/provisioning.md) und
[erste I²C-Messung](../../firmware/example-measurement.md).
Das Messbeispiel verwendet Mini; bei Weather die tatsächliche Anschlussbelegung beachten.
