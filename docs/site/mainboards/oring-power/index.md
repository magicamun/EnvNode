# ORing Power

ORing Power ist der aufgelötete Versorgungsbaustein für EnvNode-Mainboards.
Er verbindet einen 5-V-Eingang, einen 3,3-V-Regler und die Zusammenführung zweier
3,3-V-Quellen auf einer **39 × 15 mm** großen Tochterplatine.

[Bestückungs- und Bring-up-Anleitung herunterladen (PDF)](../../assets/downloads/ORing_Power_Bestueckungs_und_Bringup_Rev0.1.pdf){ .md-button }

Die Anleitung trägt die Bezeichnung 0.1. Die Platine besitzt derzeit keine formale
Revision im PCB-Titelblock; die Bezeichnung der Anleitung ist deshalb keine
zusätzliche PCB-Revisionsangabe.

## Überblick

| Merkmal | Ausführung |
| --- | --- |
| Eingänge | USB-C oder zweipoliger JST-PH, jeweils 5 V |
| Regler | LM3940IMP-3.3/NOPB |
| Zusammenführung | Zwei SS14-Schottky-Dioden |
| Ausgang | `+3V3_SYS` für das Mainboard |
| Montage | Fünf metallisierte Halblochkontakte am Platinenrand |
| Einsatz | [EnvNode Mini](../mini/index.md) und [EnvNode Weather](../weatherstation/index.md) |

## Eingänge und USB-C

USB-C J3 und JST-PH J1 liegen auf derselben `+5V`-Leitung.
Der USB-C-Anschluss ist ein GCT USB4125-GF-A-0190 mit sechs Kontakten.
CC1 und CC2 besitzen jeweils einen eigenen 5,1-kΩ-Widerstand nach GND.
Er fordert die normale 5-V-USB-C-Versorgung an; USB Power Delivery wird nicht ausgehandelt.
Es gibt keine USB-Datenleitungen.

!!! warning "5-V-Eingänge sind nicht entkoppelt"
    Verwende USB-C oder JST-PH als 5-V-Eingang. Schließe keine unabhängigen
    Netzteile gleichzeitig an beide Eingänge an, sofern diese nicht ausdrücklich
    für direkten Parallelbetrieb geeignet sind. Die ORing-Dioden trennen die
    3,3-V-Quellen, nicht die beiden 5-V-Stecker.

## Funktion der 3,3-V-Versorgung

Der LM3940 erzeugt aus `+5V` die Leitung `+3V3_EXT`.
D1 führt diese Quelle nach `+3V3_SYS`; D2 führt die vom Mainboard bereitgestellte
DevKit-Quelle `+3V3_DEVKIT` nach `+3V3_SYS`.
Die Dioden verhindern eine gegenseitige Rückspeisung über diesen ORing-Pfad.

`+3V3_SYS` liegt hinter den Schottky-Dioden. Prüfe die tatsächliche Systemspannung
unter Last anhand der Anleitung und des angeschlossenen Aufbaus.

## Verbindung zum Mainboard

| Pin | Signal | Richtung am ORing-Modul | Aufgabe |
| --- | --- | --- | --- |
| 1 | GND | Gemeinsam | Masse |
| 2 | `+5V` | Zum Mainboard | Gemeinsame 5-V-Leitung |
| 3 | `+3V3_EXT` | Zum Mainboard | Reglerausgang vor D1 |
| 4 | `+3V3_DEVKIT` | Vom Mainboard | DevKit-Quelle vor D2 |
| 5 | `+3V3_SYS` | Zum Mainboard | Zusammengeführte Systemversorgung |

Bei den hier dokumentierten Mini- und Weather-Mainboards ist der Interface-Pin
`+3V3_EXT` auf der Trägerplatine bewusst nicht angeschlossen.
Die Systemversorgung wird über `+3V3_SYS` geführt.

## Bestückung

| Bauteil | Aufgabe | Ausführung |
| --- | --- | --- |
| U1 | 3,3-V-Regler | LM3940, SOT-223 |
| D1, D2 | Quellenzusammenführung | SS14, SMA |
| R1, R2 | USB-C-CC-Widerstände | Je 5,1 kΩ |
| C1, C5 | Lokale Abblockung | Je 100 nF, 1206 |
| C2 | Reglerausgang | 10 µF Elektrolyt, polarisiert |
| C3 | 5-V-Eingang | 10 µF Keramik, 1206 |
| C4 | Systemversorgung | 47 µF Tantal, polarisiert |
| J1 | Alternativer 5-V-Eingang | Zweipoliger JST-PH |
| J3 | 5-V-USB-C-Eingang | GCT USB4125-GF-A-0190 |

Die detaillierte Zuordnung, Orientierung und Prüfung stehen in der
[PDF-Anleitung](../../assets/downloads/ORing_Power_Bestueckungs_und_Bringup_Rev0.1.pdf).
Achte insbesondere auf die Polarität von Dioden, Elektrolyt- und Tantalkondensator.

## Montage

Das Modul wird mit seinen fünf Halblochkontakten direkt auf den vorgesehenen
Träger-Footprint gelötet. Prüfe Orientierung, Freiraum unter dem Modul,
Steckerzugang und die Zugänglichkeit der Lötstellen.

Die Modulplatine benötigt metallisierte Halblöcher am Rand. Gewöhnliche ausgefräste
Bohrungen ohne Randmetallisierung sind kein gleichwertiger Ersatz; die Fertigung
muss diese Ausführung unterstützen.

## Erste Inbetriebnahme

1. Bestückung, Polarität und Lötstellen gemäß Anleitung prüfen.
2. Vor dem Einschalten Durchgang und mögliche Kurzschlüsse kontrollieren.
3. Eine geeignete 5-V-Quelle anschließen und die Versorgung nach Anleitung messen.
4. Reglerausgang und Systemspannung prüfen, anschließend das Verhalten unter Last.
5. Wenn beide 3,3-V-Quellen im Aufbau verwendet werden, Übergang und Rückstromverhalten prüfen.
6. Temperaturverhalten, USB-C-Steckverbindung und Halblochlötstellen prüfen.

Beachte dabei auch die Inbetriebnahmeanleitung des verwendeten Mainboards.

## Prüfstand

Die Quellunterlagen berichten für den 21. September 2026 über ERC ohne Fehler oder
Warnungen, DRC ohne Verstöße oder unverbundene Elemente und eine konsistente
Zuordnung der USB-C-Signale. Am 5. Oktober wurden die Dokumentationsquellen und
Pad-Netze mit der Schaltplannetzliste abgeglichen.

Diese Prüfungen bestätigen den dokumentierten Konstruktionsstand. Physische Tests
von Regelung, Quellenübergang, Rückstrom, Temperatur und Montage sind in den
vorhandenen Unterlagen noch als ausstehend geführt.
