# SSD1309 I2C Display Lab

Eigenständiger Hardwaretest für das vorhandene 2,42-Zoll-OLED (128 × 64,
Platinenaufdruck `2.42OLED-IIC VER:1.1`). Die gescannte 7-Bit-Adresse ist
`0x3C`. SSD1309 NONAME2 ist zunächst eine zu prüfende Controllerannahme.
Keine Abhängigkeit von der EnvNode-Firmware; keine NVS-Konfiguration.

## Anschluss

Lab-Vorgaben für einen klassischen ESP32 (`esp32dev`):

| Display | ESP32 | Kabelfarbe auf dem Foto |
| --- | --- | --- |
| GND | GND | Weiß |
| VDD | Versorgung gemäß Modul-Spezifikation | Gelb |
| SCL | GPIO 22 | Schwarz |
| SDA | GPIO 21 | Rot |

Pins, Adresse und Bustakt (100 kHz) stehen in `include/LabConfig.h`.
Die Versorgungsspannung ist durch das Foto nicht belegt. Am ESP32 müssen
SDA/SCL mit 3,3-V-Pegeln arbeiten; etwaige Modul-Pull-ups berücksichtigen.
Die Lab-Pins müssen auf dem Testaufbau frei sein.

## Testbild und Auswertung

Sechs feste Beispielzeilen, Font `u8g2_font_t0_11_tf`, Baselines bei
10, 20, 30, 40, 50 und 60 Pixeln. Datum und Messwerte sind Testtexte,
keine Uhr oder echten Messwerte. Rotation ist `U8G2_R0`.

Der Start prüft zunächst ein ACK an `0x3C`. Ohne ACK werden keine
Displaybefehle gesendet. Mit ACK wird die Seite einmal übertragen.
Ein ACK bestätigt lediglich einen Teilnehmer, nicht den Controllertyp oder
eine erfolgreiche Darstellung. U8g2 erhält die Adresse als `0x3C << 1`.
Im seriellen Monitor erscheinen die Pixelbreiten aller sechs Zeilen.

Prüfen: Sind alle sechs Zeilen vollständig, lesbar und richtig ausgerichtet?
Bleibt das Display trotz ACK leer oder zeigt es Müll, Controller/Variante
anhand Produktunterlagen prüfen. Ein erfolgreicher Build bestätigt die
Hardwareannahme nicht. Für 180° Rotation `U8G2_R0` durch `U8G2_R2` ersetzen.

## Build und Flash

PlatformIO: Espressif32 7.0.1, Arduino, U8g2 2.36.12.
Vom Repository-Root:

```sh
pio run -d lab/Display-SSD1309
pio device list
pio run -d lab/Display-SSD1309 -t upload --upload-port /dev/cu.usbserial-1
pio device monitor -p /dev/cu.usbserial-1 -b 115200 --filter direct
```

Falls nötig `~/.platformio/penv/bin/pio` verwenden und den Port ersetzen.
Flashen ersetzt die laufende Firmware des gewählten ESP32. Nach Öffnen des
Monitors ggf. Reset drücken. Der Nutzer hat den Lab-Test auf dem vorhandenen Modul erfolgreich geflasht und
die Darstellung bestätigt.

Referenz: [U8g2-Konstruktoren](https://github.com/olikraus/u8g2/wiki/u8g2setupcpp#ssd1309-128x64_noname2).
