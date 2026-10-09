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

## Konfiguration in der Firmware

1. Die Weboberfläche **Display** öffnen.
2. Ausgang aktivieren und den passenden Controllertyp auswählen.
3. I2C0 oder I2C1 sowie 0x3C oder 0x3D passend zur Verdrahtung setzen.
4. Anzeigeseite zuordnen und deren Zeilen mit Quellen und Formatierung konfigurieren.
5. Speichern und Vorschau sowie reale Ausgabe prüfen.

Es gibt zwei Displayausgänge und zwei konfigurierbare Seiten mit jeweils sechs Zeilen.
Jeder Ausgang erhält eine Seite; beide können dieselbe Seite anzeigen.
Zwei aktive Ausgänge dürfen nicht dieselbe Adresse am selben Bus belegen.

## Anzeigeinhalte

Die Zeilen verwenden die gemeinsame Eigenschaftsauflösung der Firmware:
Sensorwerte, Aktor- und Controller-Eigenschaften sowie Systeminformationen können
als Quellen ausgewählt werden. Eigene Values stellen weitere Eigenschaften bereit.
Boolesche Werte können eigene Texte für wahr/falsch erhalten;
Enum-Codes können in passende Anzeigetexte übersetzt werden.

## Fehlersuche

| Beobachtung | Prüfen |
| --- | --- |
| Display bleibt dunkel | Versorgung, Typ, Aktivierung, Bus und Adresse |
| I²C-Adresse fehlt beim Scan | Kabel, Pegel, Pull-ups und Moduladresse |
| Vorschau stimmt, Hardware nicht | Gewählten Treiber und reale Display-Ausführung |
| Wert fehlt | Quellzuordnung, Sensorzustand und verfügbare Eigenschaften |

[Sensoren und I²C](../sensors/i2c.md) · [Firmware-Konfiguration](../firmware/configuration.md)
