# Ich möchte messen

Du hast ein EnvNode und möchtest einen Messwert erfassen. Beginne mit der Frage,
**was du messen möchtest**. Danach wählst du einen unterstützten Sensor und richtest
ihn in der Weboberfläche ein. Die Tabellen zeigen, welche Sensoren du mit der
gemeinsamen EnvNode-Firmware einrichten kannst.

## Welche Messgröße möchtest du erfassen?

| Ich möchte … | Dafür kannst du verwenden | Was du erhältst |
| --- | --- | --- |
| Temperatur messen | BME280, SHT4x, SHTC3 oder AM2302 / DHT22 | Temperatur am Sensor. |
| Relative Luftfeuchte messen | BME280, SHT4x, SHTC3 oder AM2302 / DHT22 | Relative Feuchte in Prozent. |
| Luftdruck messen | BME280 | Örtlichen Luftdruck am Messort. |
| Temperatur, Feuchte und Luftdruck gemeinsam erfassen | BME280 | Alle drei Messgrößen mit einem Sensor. |
| Niederschlag mit einer Kippwaage erfassen | Regenmesser mit unterstütztem Kippkontakt, Implementierung **Rain Gauge** | Kippereignisse und Niederschlagszuwachs mit konfigurierten Millimetern pro Kippung. |
| Wechselstrom messen | SCT013 mit passendem Spannungsausgang und SCT013Head, Implementierung **SCT013 AC RMS** | Effektivstrom in Ampere; keine Leistung oder Energie. |
| Benetzung einer Sensorfläche erkennen | RainDetectorController mit RainDetectorHead, Implementierung **RainDetector (frequency)** | Frequenz des Sensors; mit dem zugehörigen lokalen Controller Benetzungserkennung und Heizsteuerung. |

Ein RainDetector beobachtet die Benetzung seiner Fläche. Eine Kippwaage erfasst
Niederschlag über Kippereignisse. Wähle die Kombination passend zu deiner Aufgabe;
aus der RainDetector-Frequenz entsteht keine Niederschlagsmenge in Millimetern.
Für den [RainDetector](../sensors/rain-detector/index.md) sind die Einmessung am
eigenen Aufbau und die mechanische Ausführung für den vorgesehenen Einsatz erforderlich.

## Welchen Temperatur- und Feuchtesensor nehme ich?

**BME280** passt, wenn du zusätzlich Luftdruck erfassen möchtest. Für den ersten
Aufbau zeigt [Dein erstes EnvNode](../getting-started/index.md) den Weg bis zum
Messwert. Die [BME280-Einrichtung](../firmware/sensors.md#bme280-konfigurieren)
ist auch mit Bildern dokumentiert.

**SHT4x oder SHTC3** liefern Temperatur und relative Luftfeuchte. Sie werden in der
Firmware als unterschiedliche Implementierungen ausgewählt. Ein
[SHT4x-Beispiel](../firmware/example-measurement.md) führt bis zur MQTT-Ausgabe.

**AM2302 / DHT22** liefert ebenfalls Temperatur und relative Luftfeuchte und nutzt
einen geeigneten digitalen Ein-/Ausgang. Wenn du bereits einen solchen Sensor
besitzt, wähle in der Oberfläche die Implementierung **AM2302 / DHT22**.

Bei jedem Typ muss das konkrete Sensorboard zur Versorgung und zu den Signalpegeln
des EnvNode-Anschlusses passen. Der Name einer Sensorfamilie allein garantiert
keine passende Ausführung des gesamten Moduls.

## Welche zusätzlichen Werte kann EnvNode bereitstellen?

Einige Werte werden aus vorhandenen Messungen beziehungsweise Standort und Zeit
berechnet. Dafür ist kein zusätzlicher Messfühler nötig.

| Gewünschter Wert | Voraussetzung | Bedeutung |
| --- | --- | --- |
| Taupunkt | Gültige Temperatur und relative Feuchte desselben unterstützten Temperatur-/Feuchtesensors | Aus diesen Messwerten abgeleitete Taupunkttemperatur. |
| Absolute Luftfeuchte | Gültige Temperatur und relative Feuchte desselben unterstützten Temperatur-/Feuchtesensors | Aus diesen Messwerten abgeleiteter Wassergehalt der Luft. |
| Auf Meereshöhe reduzierter Luftdruck | BME280 und konfigurierte Sensorhöhe; optional eine passende lokale Temperaturquelle | Vergleichbarer abgeleiteter Druckwert zusätzlich zum örtlichen Druck. |
| Sonnenstand | Konfigurierter Breitengrad und Längengrad, synchronisierte Zeit und Implementierung **Solar Position** | Berechnete Sonnenrichtung und Sonnenhöhe. Keine Messung der Helligkeit oder Solarstrahlung. |

Für den Meereshöhendruck beschreibt [Standort und Luftdruck](../firmware/geo-position-pressure.md)
die Eingaben und die Bedeutung ungültiger beziehungsweise abgeleiteter Werte.
Die [Einrichtung abgeleiteter Werte](../firmware/derived-measurements.md) erklärt
Taupunkt, absolute Luftfeuchte und Sonnenstand genauer. Berechnete Werte und
Simulationen sind von echten Sensormessungen zu unterscheiden.

## Vom ausgewählten Sensor zum Messwert

Die [Inbetriebnahmeübersicht](../commissioning/index.md) führt zur passenden
Anleitung. Für die häufigsten Aufgaben kannst du direkt einsteigen:

| Deine Auswahl | Einrichten und prüfen |
| --- | --- |
| BME280, SHT4x oder SHTC3 | [I²C-Sensoren](../commissioning/i2c-sensors.md) |
| AM2302 / DHT22 | [Sensorformular und GPIO-Auswahl](../firmware/sensors.md) |
| Regenwippe | [Kippkontakt, Kalibrierwert und Zählprüfung](../commissioning/rain-gauge.md) |
| RainDetector | [Frequenzsensor, Heizaktor und Controller](../commissioning/rain-detector.md) |
| SCT013 | [SCT013 einrichten und prüfen](../sensors/sct013/commissioning.md) |

Die [Firmware-Anleitung](../firmware/index.md) erklärt die gemeinsamen
Bedienelemente. Aufbauanleitungen brauchst du, wenn du Hardware selbst bestückst
oder deren technische Umsetzung prüfst.

## Wo sollen deine Messwerte erscheinen?

Du kannst zunächst alle Messwerte in der **Weboberfläche** prüfen. Für eine Anzeige
am Gerät richtest du ein [Display](../firmware/values-displays.md) ein. Für die
Weitergabe an ein anderes System folgt die [MQTT-Einrichtung](../firmware/mqtt.md),
beispielsweise für Home Assistant. Die Sensorwahl bleibt dabei dieselbe.

## Druck und Füllstand mit einer 4–20-mA-Sonde

[AnalogHydroPressure](../modules/analog-hydro-pressure/index.md) ist ein eigenes
Hardwareprojekt. Der Messpfad ist noch nicht als fertige Auswahl in der gemeinsamen
Firmware integriert. Der technische Aufbau ist dokumentiert; eine Software-
Inbetriebnahme folgt mit der Unterstützung.

Die [Sensorhardware](../sensors/index.md) enthält technische Details;
die [Kompatibilitätsübersicht](../compatibility.md) ordnet die Kombinationen ein.
