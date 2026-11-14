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
4. Prüfe vorhandene Pull-ups auf allen beteiligten Boards. Die Mini-Pull-ups werden
   über normalerweise offene Lötjumper zugeschaltet.
5. Verbinde den Sensor bei ausgeschalteter Versorgung.

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
