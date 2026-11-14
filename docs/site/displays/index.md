# Displays

Die Firmware unterstützt I²C-Textanzeigen mit 128×64 Pixeln über U8g2:

| Controller | Auflösung | I²C-Adressen |
| --- | --- | --- |
| SSD1309 | 128×64 | 0x3C oder 0x3D |
| SSD1306 | 128×64 | 0x3C oder 0x3D |
| SH1106 | 128×64 | 0x3C oder 0x3D |

Diese Liste beschreibt die eingebauten Treiber. Sie ist keine Zusage, dass jedes
Modul mit gleichem Controllernamen elektrisch und mechanisch passend ist.
SPI-Ausführungen oder andere Auflösungen sind nicht durch diese Konfiguration abgedeckt.

## Anschluss

Display spannungsfrei anschließen und Versorgung, Logikpegel und Steckerbelegung prüfen.
Der Mainboard-I²C-Anschluss führt GND, +3V3_SYS, SDA und SCL.
Vorhandene Pull-ups und die tatsächliche Adresse des Displays berücksichtigen.

## Was möchtest du tun?

| Dein Ausgangspunkt | Hier geht es weiter |
| --- | --- |
| Display anschließen | Anschlussbedingungen oben und [HW-Schnittstellen](../interfaces/index.md) |
| Geprüftes Display einrichten | [Displayausgänge und Inhaltsseiten](../firmware/values-displays.md#displays) |
| Messwerte oder Zustände anzeigen | [Quellen, Formate und Beispiel mit zwei Displays](../firmware/values-displays.md) |

Die Firmware-Anleitung ist die gemeinsame Quelle für die Einrichtung.

## Fehlersuche

| Beobachtung | Prüfen |
| --- | --- |
| Display bleibt dunkel | Versorgung, Typ, Aktivierung, Bus und Adresse |
| I²C-Adresse fehlt beim Scan | Kabel, Pegel, Pull-ups und Moduladresse |
| Vorschau stimmt, Hardware nicht | Gewählten Treiber und reale Display-Ausführung |
| Wert fehlt | Quellzuordnung, Sensorzustand und verfügbare Eigenschaften |

[Sensoren und I²C](../sensors/i2c.md) · [Firmware-Konfiguration](../firmware/configuration.md)
