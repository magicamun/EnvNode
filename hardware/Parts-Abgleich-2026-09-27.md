# Bauteilabgleich und Verbrauch – 27.09.2026

Auftrag vom 22.09.2026, am 27.09.2026 abgeschlossen. Verbrauch anhand der vor der Umstellung gespeicherten Kits gebucht. Zwei EnvNode Mini Rev. 0.3 (ein funktionaler Aufbau, ein Ausschuss), jeweils ohne USB-C-Buchse, mit EEPROM und 5,1-kΩ-Widerständen; ein DuoRelay mit EEPROM, dessen 100-nF-Kondensator und Schraubklemmen; ein zusätzlicher zerstörter EEPROM; ein alter RainDetector. Mechanische Gehäuseanschlüsse wurden durch „bestückt“ nicht als verbraucht interpretiert.

## Verbrauchsbuchung

Negative Werte sind ausdrücklich genehmigte Bestandsdifferenzen. Keine nicht belegten Wareneingänge oder Umbuchungen auf Ersatzartikel vorgenommen. Bereits bestellte Mengen bleiben unverändert.

| Part | Bauteil | Vorher | Verbrauch | Danach |
|---|---|---:|---:|---:|
| ENP-00006 | 100 nF Keramik SMD | 50 | 10 | 40 |
| ENP-00007 | 10 uF Keramik SMD | 10 | 2 | 8 |
| ENP-00008 | 47 uF 10V Tantal | 14 | 2 | 12 |
| ENP-00056 | Elektrolytkondensator 10 µF SMD | 10 | 2 | 8 |
| ENP-00055 | Schottky-Diode SS14, SMA | 20 | 4 | 16 |
| ENP-00012 | JST - Stiftleiste, gerade, 1x2-polig - PH | 6 | 2 | 4 |
| ENP-00054 | Buchsenleiste 2x7, 2,54 mm, vertikal | 0 | 4 | -4 |
| ENP-00026 | JST-SH Stiftleiste, 4-polig, horizontal | 0 | 4 | -4 |
| ENP-00005 | 4.7 kOhm 1% SMD 1206 | 62 | 8 | 54 |
| ENP-00010 | ESP32-DevKitC | 3 | 2 | 1 |
| ENP-00102 | I²C-EEPROM 32 Kbit, SOIC-8 | 10 | 4 | 6 |
| ENP-00009 | LDO 3.3V SOT-223 | 3 | 2 | 1 |
| ENP-00103 | EnvNode Mini PCB Rev 0.3 | 0 | 2 | -2 |
| ENP-00090 | SMD-Widerstand 5,1 kOhm, 5 %, 1206 | 0 | 4 | -4 |
| ENP-00033 | Schnelle Schaltdiode 1N4148W | 50 | 2 | 48 |
| ENP-00064 | LED SMD 1206, rot | 20 | 2 | 18 |
| ENP-00059 | Stiftleiste 2x7, 2,54 mm, vertikal, Bottom-Mount | 8 | 1 | 7 |
| ENP-00034 | Schraubklemme 1x3, horizontal | 0 | 2 | -2 |
| ENP-00023 | Steck-Printrelais, 1 CO, 6 A, 5 V DC sensitiv | 4 | 2 | 2 |
| ENP-00035 | NPN-Transistor BC817 | 50 | 2 | 48 |
| ENP-00036 | Widerstand 1 kΩ SMD | 20 | 2 | 18 |
| ENP-00016 | SMD-Widerstand, 1206, 100 kOhm, 250 mW, 0,1% | 14 | 4 | 10 |
| ENP-00065 | Widerstand 1,5 kΩ SMD | 0 | 2 | -2 |
| ENP-00104 | DuoRelay FullSize PCB Rev 0.3 | 0 | 1 | -1 |
| ENP-00019 | Stiftleiste, 8-polig, Raster 2,54mm | 0 | 2 | -2 |
| ENP-00003 | PinSocket 1x4, 2.54 mm | 4 | 2 | 2 |
| ENP-00013 | MOSFET, N-Ch 30V 5,7A 0,018R, SOT-23 | 5 | 1 | 4 |
| ENP-00017 | SMD-Widerstand, 1206, 100 Ohm, 250 mW, 5% | 50 | 1 | 49 |
| ENP-00018 | SMD-Widerstand, 1206, 10 kOhm, 250 mW, 0,1% | 9 | 1 | 8 |
| ENP-00020 | Timer-IC, Typ 555, SO-8 | 3 | 1 | 2 |
| ENP-00053 | RainDetector PCB | 5 | 1 | 4 |

## Planung

| Kit | Revision | Menge |
|---|---|---:|
| EnvNode Mini | 0.6 | 2 |
| Weatherstation | 0.1 | 2 |
| ORing Power | 0.1 | 5 insgesamt |
| RainDetectorHead | 0.1 | 3 |
| RainDetectorController | 0.3 | 2 |
| AnalogHydroPressure | 0.4 | 2 |

Die fünf ORing-Module enthalten vier Module für die Mainboards und ein Reservemodul. Unterbaugruppen stehen in der BOM, werden aber ausschließlich über ihr eigenes Kit in der Bestellliste gezählt. AnalogHydroPressure wird erst nach Eingang der Rev.-0.4-PCBs aufgebaut. Seine bisherigen DNP-Positionen bleiben DNP.

Die sechs angekündigten PCB-Fertigungslose zu jeweils fünf Stück wurden nicht zusätzlich als Wareneingang oder Bestellung gebucht. Vorhandene PCB-Einträge bleiben erhalten. ENP-00121 (Weatherstation) enthält noch kopierte Mini-Daten bei Revision, Footprint/Pfad, MPN und Notizen; diese sind beim eigenen PCB-Nachtrag zu korrigieren. PCB-Zeilen werden als „PCB-Nachtrag“ gekennzeichnet und nicht erneut als Kaufvorschlag ausgegeben.

## Bauteile und Quellen

- Vertikaler JST-SH: ENP-00122, JST BM04B-SRSS-TB, DigiKey 455-BM04B-SRSS-TBCT-ND. Vier pro Mini und acht pro Weatherstation, insgesamt 24. Horizontaler ENP-00026 bleibt ein separater Artikel. [JST-Datenblatt](https://www.jst.com/wp-content/uploads/2021/01/eSH.pdf), [DigiKey](https://www.digikey.de/de/products/detail/jst-sales-america-inc/BM04B-SRSS-TB/926696).
- USB-C ORing: ENP-00117, GCT USB4125-GF-A-0190. Mouser-Artikelnummer korrigiert auf 640-USB4125-GF-A-190. Fünf benötigt. [Mouser](https://www.mouser.de/de/ProductDetail/GCT/USB4125-GF-A-0190), [GCT-Zeichnung](https://gct.co/files/drawings/usb4125.pdf).
- ORing JST-PH SMD: ENP-00119. Herstellerbezeichnung B2B-PH-SM4-TBT, Reichelt-Artikel JST PH2P SM4. KiCad-Footprintname bleibt B2B-PH-SM4-TB. [JST](https://jst.de/produkt/3424/692450-00), [Reichelt](https://www.reichelt.de/de/de/shop/produkt/jst_-_stiftleiste_rm_2_mm_1x2_polig_gerade_-_ph-190546).
- Lieferantendaten wurden am 22.09.2026 recherchiert. Preise sind keine aktuellen Angebote. Offene Preise bleiben leer und werden in der Bestellliste ausgewiesen.

## Vorläufige Fehlteile

Berechnet mit erfasstem Bestand, unveränderten offenen Bestellungen und bereits freigegebenen Alternativen. Negative Bestandsdifferenzen erhöhen den Vorschlag; tatsächliche Wareneingänge und verbaute Ersatzartikel vor Kauf klären. Manuelle Bestellmengen wurden erhalten und keine Bestellung ausgelöst.

| Part | Bauteil | Lieferant | Artikelnummer | Bedarf | Bestellvorschlag |
|---|---|---|---|---:|---:|
| ENP-00009 | LDO 3.3V SOT-223 | Reichelt | LM 3940 IMP-3 | 5 | 6 |
| ENP-00010 | ESP32-DevKitC | Noch festlegen | offen | 4 | 3 |
| ENP-00027 | 16-Bit ADC mit I²C | DigiKey | 296-24934-1-ND | 2 | 2 |
| ENP-00054 | Buchsenleiste 2x7, 2,54 mm, vertikal | Reichelt | MPE 094-2-014 | 6 | 8 |
| ENP-00106 | Schottky-Diode B260S1F-7 | Reichelt | B260S1F-7 | 2 | 2 |
| ENP-00080 | WEIPU SP13 Gehäusebuchse, 2-polig | Noch festlegen | offen | 2 | 2 |
| ENP-00081 | WEIPU SP13 Kabelstecker, 2-polig | Noch festlegen | offen | 2 | 2 |
| ENP-00084 | WEIPU SP13 Gehäusebuchse, 4-polig | Noch festlegen | offen | 2 | 2 |
| ENP-00085 | WEIPU SP13 Kabelstecker, 4-polig | Noch festlegen | offen | 2 | 2 |
| ENP-00111 | SMD-Stiftleiste 2x10, Zuschnitt auf 2x2 | Reichelt | SL 2X10G SMD2,54 | 3 | 3 |
| ENP-00115 | SMD-Heizwiderstand 15 Ohm, 1 %, 1206 | Reichelt | WAL WR12X15R0FTL | 60 | 60 |
| ENP-00116 | Buchsenleiste 2x2, 2,54 mm, THT | Reichelt | BKL 10120957 | 4 | 4 |
| ENP-00117 | USB-C-Buchse, 6-polig, Power-only, Top-Mount | Mouser | 640-USB4125-GF-A-190 | 5 | 5 |
| ENP-00119 | JST-PH Stiftleiste, 2-polig, vertikal, SMD | Reichelt | JST PH2P SM4 | 5 | 5 |
| ENP-00122 | JST-SH Stiftleiste, 4-polig, vertikal | DigiKey | 455-BM04B-SRSS-TBCT-ND | 24 | 24 |
| ENP-00110 | SMD-Widerstand 5,1 kOhm, 1 %, 1206 | DigiKey | 541-5.10KFCT-ND | 10 | 8 |
| ENP-00025 | JST-PH Stiftleiste, 4-polig, vertikal | Reichelt | JST PH4P ST | 20 | 20 |

## Prüfung

BOMs von Mini und Weatherstation mit aktuellen KiCad-Exporten abgeglichen. Bedarf unabhängig aus BOM-Mengen und Planstückzahlen nachgerechnet. EEPROM-Verbrauch sowie JST-SH-, USB-C-, Regler- und Heizwiderstandsbedarf geprüft. Formeln und exportierte Datei auf Fehler geprüft.
