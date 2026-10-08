# AnalogHydroPressure

AnalogHydroPressure ist ein FullSize-Modul für eine Zweileiter-Drucksonde mit
**4–20-mA-Ausgang**. Es erzeugt lokal die Sondenversorgung und digitalisiert den
Schleifenstrom. Diese Seite beschreibt **Revision 0.5**.

[Bestückungs- und Bring-up-Anleitung (PDF)](../../assets/downloads/AnalogHydroPressure_Bestueckungs_und_Bringup_Rev0.5.pdf){ .md-button }

## Messpfad und Versorgung

Ein LT8330 erzeugt aus den 5 V des Modulslots die lokale Versorgung von etwa 24 V.
Der Schleifenstrom erzeugt am 150-Ω-Shunt (0,1 %) eine Spannung, die gefiltert,
begrenzt und am ADS1115-Eingang AIN0 gemessen wird.

| Schleifenstrom | Nominale Shuntspannung |
| --- | --- |
| 4 mA | 0,600 V |
| 12 mA | 1,800 V |
| 20 mA | 3,000 V |

Diese Werte prüfen den elektrischen Messpfad. Der zugehörige Druckbereich hängt
von der Sonde ab; Nullpunkt und Endwert müssen mit der konkreten Sonde kalibriert werden.

## Modulanschluss

| Pin | Signal | Funktion |
| --- | --- | --- |
| 1 | GND | Versorgung und Messreferenz |
| 2 | +3V3_SYS | ADS1115 und optionale Identifikation |
| 3 | EEPROM_A0 | Optionales EEPROM |
| 4 | +5V | Eingang des 24-V-Boosts |
| 5 / 6 | I2C0 SDA / SCL | Wählbarer ADS1115-Bus; optionales EEPROM fest |
| 7 / 8 | I2C1 SDA / SCL | Wählbarer ADS1115-Bus |
| 9 | AUX_GPIO1 | Nicht verwendet |
| 10 | AUX_GPIO2 | Boost-Enable |

SPI wird nicht verwendet. Eine zusätzliche SCT013-Eingangsschaltung
ist auf diesem Modul nicht implementiert.

## Konfiguration vor dem Einschalten

### ADS1115-Adresse

Genau einen Adressjumper schließen:

| Jumper | Adresse |
| --- | --- |
| JP1 | 0x48 |
| JP2 | 0x49 |
| JP3 | 0x4A |
| JP4 | 0x4B |

Bei mehreren Modulen auf demselben ausgewählten Bus müssen die ADC-Adressen verschieden sein.
Die ADS1115-Adresse ist unabhängig von der optionalen Module-Identity-Adresse.

### I²C-Buswahl mit JP5 und JP6

JP5 wählt SDA, JP6 wählt SCL. Beide dreipoligen Lötjumper müssen gemeinsam
auf denselben Bus gesetzt werden:

| Bus | JP5 (SDA) | JP6 (SCL) |
| --- | --- | --- |
| I2C0 | Pins 1–2 brücken | Pins 1–2 brücken |
| I2C1 | Pins 2–3 brücken | Pins 2–3 brücken |

Ohne beide Brücken ist der ADS1115 nicht am I²C-Bus erreichbar.
Keine gemischte Buswahl verwenden und niemals alle drei Pins eines Jumpers
verbinden: Damit würden I2C0 und I2C1 zusammengeschaltet.
Die Jumper wählen den Datenbus, nicht die 5-V-Versorgung.
Das optionale Identity-EEPROM bleibt unabhängig davon fest an I2C0.

### Boost-Enable

AUX_GPIO2 HIGH aktiviert den Boost, LOW deaktiviert ihn. R7 (100 kΩ) zieht den
Enable-Eingang nach GND.

| Mainboard / Slot | Boost-Enable |
| --- | --- |
| Mini A | GPIO13 |
| Mini B | GPIO14 |
| Weather A | GPIO13 |

!!! warning "Restspannung berücksichtigen"
    Auch nach dem Abschalten des Boosts kann C9 Restspannung halten.
    Vor Arbeiten die 24-V-Leitung messen. Die erste Inbetriebnahme erfolgt
    strombegrenzt und ohne angeschlossene Sonde.

## Sondenanschluss und optionale Positionen

J3 verbindet die Zweileiter-4–20-mA-Sonde. Bei Ersatzsonden Versorgung,
Polung und Anschlussbelegung anhand des jeweiligen Datenblatts prüfen.
Der optionale Messheader J2 führt GND, +24V, LOOP_RETURN und AIN0.

J2 sowie das Identity-EEPROM U3 und dessen Abblockkondensator C1 sind laut
aktueller Anleitung DNP, also nicht zu bestücken. Ein nachträglich bestücktes
EEPROM nutzt I2C0 und die slotabhängige Adresse `0x52` oder `0x53`.
Die Messfunktion erfordert dieses EEPROM nicht.

## Bestückung und Inbetriebnahme

Die [PDF-Anleitung](../../assets/downloads/AnalogHydroPressure_Bestueckungs_und_Bringup_Rev0.5.pdf)
enthält Bauteilwerte, Orientierung und den vollständigen Prüfablauf.

1. Feine IC-Pins, Diodenpolung und Bottom-Mount-Stecker prüfen.
2. Kurzschlüsse ausschließen; genau einen ADC-Adressjumper und JP5 und JP6 synchron auf den gewünschten I²C-Bus setzen.
3. AUX_GPIO2 zunächst LOW halten und ohne Sonde 5 V strombegrenzt zuführen.
4. 3,3-V- und 5-V-Versorgung prüfen; Boost anschließend gezielt einschalten.
5. 24-V-Ausgang, Schaltverhalten, Ripple und Temperatur prüfen.
6. ADS1115 auf der gewählten Adresse am ausgewählten I²C-Bus suchen.
7. Mit einer geeigneten Stromquelle 4, 12 und 20 mA prüfen und ADC-Rohwerte dokumentieren.
8. Versorgung unter Last prüfen und anschließend mit der vorgesehenen Sonde kalibrieren.

## Firmware-Anbindung

Die Hardware benötigt getrennt eine ADS1115-Messanbindung und die passende
Ansteuerung von AUX_GPIO2 für die Sondenversorgung. Module Identity aktiviert
weder ADC-Treiber noch Boost automatisch.

Die vorhandene Firmware-Dokumentation führt die ADS1115-Integration noch als
zurückgestellt. Diese Seite beschreibt deshalb den Hardware- und Prüfstand;
eine fertige Drucksensor-Konfiguration in der gemeinsamen Firmware ist damit
nicht zugesagt. Eine konkrete Anleitung folgt nach Abgleich des Firmwarestands
und Prüfung des vollständigen Messpfads.

## Fehlerbilder

| Beobachtung | Prüfen |
| --- | --- |
| Keine 24 V | AUX_GPIO2, 5-V-Versorgung und Boost-Bauteile |
| ADS1115 nicht erreichbar | 3,3 V, synchrone JP5/JP6-Buswahl, IC-Orientierung und Adressjumper |
| Messwert fehlt | Sondenschleife, Versorgung, Shunt und ADC-Kanal |
| Instabile Messwerte | Ripple, Masseführung, Steckkontakte und Sensorleitung |

## Prüfstand

Für den 28. September 2026 sind ERC ohne Fehler mit zehn bekannten Warnungen,
DRC ohne Verstöße und saubere Schaltplan/PCB-Parität dokumentiert.
Am 5. Oktober wurden Anschlussbelegung und Boost-Enable gegen Netzliste und
PCB-Padnetze abgeglichen. Dies ersetzt keine physische Boost-, Last-, Temperatur-,
EMV- oder Kalibrierprüfung; diese sind in den Unterlagen noch offen.

[Mainboards](../../mainboards/index.md) · [Firmware](../../firmware/index.md)
