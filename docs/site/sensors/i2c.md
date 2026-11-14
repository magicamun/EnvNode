# I²C-Sensoren

Normale I²C-Sensoren können direkt an den I²C-Anschlüssen des Mainboards betrieben werden.
Auf EnvNode Mini stehen I2C0 und I2C1 mit jeweils zwei vierpoligen JST-SH-Anschlüssen bereit.

## Anschluss vorbereiten

1. Prüfe Versorgungsspannung und Logikpegel des Sensorboards. Die Mini-Anschlüsse
   führen GND, 3,3 V, SDA und SCL.
2. Prüfe die Pinbelegung des Kabels und beider Stecker. Ein passender Steckertyp allein
   garantiert keine passende Verdrahtung.
3. Ermittle den verwendeten Bus und die Sensoradresse. Auf I2C0 sind `0x50` für die
   Board Identity sowie `0x52` und `0x53` für optionale Module Identity vorgesehen.
4. Prüfe vorhandene Pull-ups auf Mainboard und Sensorboards. Die Zuschaltung
   hängt von der Platinenrevision ab; siehe die Zuordnung unten.
5. Verbinde den Sensor bei ausgeschalteter Versorgung.

## Pull-ups passend zur Revision

SDA und SCL benötigen wirksame Pull-ups zur passenden 3,3-V-Versorgung.
Mainboard und Sensorboards können gleichzeitig Widerstände bestückt haben;
entscheidend ist ihre gemeinsame Wirkung am tatsächlich verwendeten Bus.

| Mainboard / Revision | Zuschaltung der Mainboard-Pull-ups | Technische Anleitung |
| --- | --- | --- |
| Mini 0.8.1 | Normalerweise offene Lötjumper JP1–JP4 | [Historische Aufbauanleitung](../mainboards/mini/revisions/0.8.1.md) |
| Mini 0.9.0 und 0.10.0 | Steckbare Jumperkappen; J3/J4 für I2C0, J5/J6 für I2C1 | [Mini: Pull-ups](../mainboards/mini/revisions/0.9.0.md#steckbare-i2c-pull-ups); [Revisionsstand 0.10.0](../mainboards/mini/revisions/0.10.0.md) |
| Weather 0.3.0 | Steckbare Jumperkappen; J2/J3 für I2C0, J4/J5 für I2C1 | [Weather: Pull-ups](../mainboards/weatherstation/revisions/0.3.0.md#steckbare-i2c-pull-ups) |
| Weather 0.4 | Steckbare Jumperkappen; J1/J2 für I2C0, J3/J4 für I2C1 | [Revisionsstand 0.4](../mainboards/weatherstation/revisions/0.4.md) |

Beide Pull-ups eines Busses gemeinsam zuschalten, wenn dieser Bus seine Pull-ups
vom Mainboard erhalten soll. Bei ausreichenden Pull-ups auf den angeschlossenen
Sensorboards sind keine zusätzlichen Mainboard-Pull-ups nötig. Kappen nur
spannungsfrei umstecken und historische Referenzbezeichnungen nicht auf andere
Revisionen übertragen. Die obige Liste gilt für die genannten Platinenstände;
bei anderen Revisionen ist deren eigene Anschlussdokumentation maßgeblich.

## Unterstützte I²C-Treiber

| Implementierung | Messgrößen | In der Firmware zugelassene Adressen |
| --- | --- | --- |
| BME280 | Temperatur, relative Feuchte, Luftdruck | 0x76 oder 0x77 |
| SHT4x | Temperatur und relative Feuchte | 0x44 |
| SHTC3 | Temperatur und relative Feuchte | 0x70 |

Bus I2C0 oder I2C1 passend zum Aufbau wählen. Die Adresse muss zum
Sensorboard und zum Treiber passen. Weitere Familien wie ADS1115 oder BMP390
sind nicht automatisch durch diese Liste unterstützt.

## Weiter zur Software-Inbetriebnahme

[Passenden I²C-Sensor einrichten und Messwerte prüfen](../commissioning/i2c-sensors.md).
Ein beliebiger I²C-Sensor ist nicht automatisch unterstützt; die Implementierung
muss zum Sensor passen.

[Anschlüsse und Pinouts](../interfaces/index.md)
