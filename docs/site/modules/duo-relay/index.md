# DuoRelay

DuoRelay ist ein FullSize-Modul mit zwei unabhängig gesteuerten 5-V-Wechslerrelais.
Diese Seite beschreibt **Revision 0.3**.

[Bestückungs- und Bring-up-Anleitung (PDF)](../../assets/downloads/DuoRelay_Bestueckungs_und_Bringup_Rev0.3.pdf){ .md-button }

## Überblick

| Merkmal | Ausführung |
| --- | --- |
| Relais | Zwei Finder 34.51.7.005.0010 mit 5-V-Spulen |
| Ansteuerung | AUX_GPIO1 und AUX_GPIO2, aktiv HIGH |
| Treiber | Je BC817, Basiswiderstand, Pull-down und Freilaufdiode |
| Anzeige | Eine LED pro Kanal |
| Modulformat | FullSize, 38 × 64 mm |
| Anschluss | Von unten bestückte 2 × 7-Stiftleiste |
| Identifikation | Optionales 24LC32-EEPROM U2 mit C1; im dokumentierten Kit DNP |

## Modulanschluss

| Pin | Signal | Funktion |
| --- | --- | --- |
| 1 | GND | Masse der Steuerseite |
| 2 | +3V3_SYS | Optionale EEPROM-Versorgung |
| 3 | EEPROM_A0 | Slotabhängige EEPROM-Adresse |
| 4 | +5V | Relais- und LED-Versorgung |
| 5 / 6 | I2C0 SDA / SCL | Optionales EEPROM |
| 9 | AUX_GPIO1 | Kanal 1 |
| 10 | AUX_GPIO2 | Kanal 2 |

I2C1 und SPI werden nicht verwendet. Die 100-kΩ-Pull-downs sorgen bei offenen
Steuereingängen für einen definierten Aus-Zustand. HIGH aktiviert den jeweiligen
Treiber; LOW lässt das Relais abfallen.

## Relaiskontakte

| Kanal | Klemme | Pin 1 | Pin 2 | Pin 3 |
| --- | --- | --- | --- | --- |
| 1 | J3 | NO | COM | NC |
| 2 | J4 | NC | COM | NO |

COM ist der gemeinsame Kontakt. Ohne Ansteuerung ist COM mit NC verbunden;
bei angezogenem Relais mit NO. Die Klemmen sind gegensinnig orientiert:
Belegung immer anhand Aufdruck und spannungsfreier Durchgangsmessung prüfen.
Die LED zeigt die elektrische Ansteuerung, nicht die tatsächliche mechanische Kontaktstellung.

!!! warning "Erste Inbetriebnahme mit Kleinspannung"
    Die Relaisdaten und Leiterplattenabstände sind keine Freigabe des fertigen Aufbaus
    für Netzspannung. Die dokumentierte Erstinbetriebnahme erfolgt mit sicherer
    Kleinspannung. Last, Klemmen, Absicherung, Gehäuse und Berührungsschutz müssen
    für die konkrete Anwendung geeignet sein.

## Firmware-Zuordnung

Konfiguriere je Relais einen GPIO-On/Off-Aktor mit aktivem HIGH-Pegel und sicherem
Aus-Zustand. Die tatsächlichen GPIOs hängen vom Mainboard und Slot ab:

| Mainboard / Slot | Kanal 1 | Kanal 2 |
| --- | --- | --- |
| Mini A | GPIO33 | GPIO13 |
| Mini B | GPIO32 | GPIO14 |
| Weather A | GPIO33 | GPIO13 |

Prüfe die angezeigte Ressourcenbelegung vor dem Schalten.
Ein optionales EEPROM verwendet I2C0 mit Adresse `0x52` in Slot A oder `0x53` in Slot B.
U2 und C1 sind laut aktueller Anleitung nicht zu bestücken (DNP).
Ein gültiger Identifikationsdatensatz erzeugt keine Relais-Aktoren automatisch.

## Bestückung und Inbetriebnahme

Die [PDF-Anleitung](../../assets/downloads/DuoRelay_Bestueckungs_und_Bringup_Rev0.3.pdf)
enthält die Bauteilwerte und den vollständigen Ablauf.

1. Dioden, LEDs und Transistoren polaritätsrichtig bestücken; J1 von unten montieren.
2. Lötstellen, Kurzschlüsse und Trennung zwischen Kontakt- und Steuerseite prüfen.
3. Spannungsfrei COM/NC und COM/NO beider Kanäle prüfen.
4. Ohne externe Last starten und beide Steuereingänge LOW halten: Relais und LEDs bleiben aus.
5. Jeden Kanal einzeln HIGH und wieder LOW schalten; LED und Kontaktzustand prüfen.
6. Beide Kanäle gleichzeitig prüfen und 5-V-Versorgung sowie Stromaufnahme beobachten.
7. Erst danach eine geeignete Kleinspannungs-Testlast anschließen.

## Fehlerbilder

| Beobachtung | Prüfen |
| --- | --- |
| Relais bleibt aus | 5 V, GPIO-Zuordnung, Treiber und Spule |
| LED leuchtet, Kontakt schaltet nicht | Spulenspannung unter Last, Lötstellen und Relais |
| Relais zieht beim Boot an | Pull-down, Boot-Pegel und Aktorkonfiguration |
| Kontaktzustand vertauscht | Gegensinnige Orientierung von J3 und J4 |

## Prüfstand

Die Anleitung wurde am 5. Oktober 2026 gegen Schaltplan-Netzliste und PCB-Padnetze
abgeglichen. Dies war keine neue vollständige ERC/DRC- oder physische Prüfung.
Lastqualifikation und anwendungsbezogene Freigaben sind in den Unterlagen noch offen.

[Mainboards](../../mainboards/index.md) · [Firmware](../../firmware/index.md)
