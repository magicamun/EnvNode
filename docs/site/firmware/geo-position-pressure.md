# GeoPosition und Luftdruck auf Meereshöhe

Ab **Firmware 0.6.0** kann EnvNode Standortdaten speichern und neben dem örtlich
gemessenen Luftdruck einen eigenen, abgeleiteten Meereshöhendruck bereitstellen.
Der örtliche Druck bleibt unverändert. Die Erweiterung wurde auf dem Gerät geprüft.

## Standort eingeben

Unter **Gerät (Device) → Standort (optional)** die bekannten Angaben eintragen:

| Angabe | Format und Bereich | Bedeutung |
| --- | --- | --- |
| Latitude / Breitengrad | Dezimalgrad, −90 bis +90 | Nord positiv, Süd negativ |
| Longitude / Längengrad | Dezimalgrad, −180 bis +180 | Ost positiv, West negativ |
| Sensorhöhe | Meter, −1000 bis +10000 | Tatsächliche Sensorhöhe über Meereshöhe einschließlich Gebäude- oder Montagehöhe |

Die Eingabe folgt ab **Firmware 0.6.1** der unter **Sprache/Zeit** gewählten Locale:
Deutsch verwendet **Komma**, beispielsweise `48,137154` und `11,576124`;
Englisch verwendet **Punkt**, beispielsweise `48.137154` und `11.576124`.
Auch Sensorhöhe und Temperatur-Maximalalter verwenden dieses Dezimaltrennzeichen.
Keine Tausendertrennzeichen und keine Grad/Minuten/Sekunden-Schreibweise verwenden.
Die internen Zahlenwerte sowie MQTT-, NVS- und Backup-Formate bleiben locale-neutral.
In Firmware 0.6.0 verwenden die Eingabefelder noch das neutrale Punktformat.
Unbekannte Angaben leer lassen: **0 m ist eine gültige Höhe**, kein Platzhalter
für „unbekannt“. Alle drei Felder sind unabhängig optional. Für die Druckberechnung
wird ausschließlich die Sensorhöhe verwendet; Latitude und Longitude gehen nicht
in die Formel ein. Ohne konfigurierte Höhe ist der Meereshöhendruck ungültig.

## Lokale Temperaturquelle wählen

Unter **Berechnung des Meereshöhendrucks** eine Außentemperaturquelle und das
maximale Alter ihres Messwerts auswählen. Voreingestellt sind **60 Sekunden**.

Die Auswahl enthält aktivierte lokale Sensoren mit aktiviertem Messplan und einem
Temperaturkanal, bezeichnet durch Slot und Namen. Simulierte Quellen sind markiert.
Eine BME280-Temperatur im Gehäuse muss nicht die Außentemperatur wiedergeben;
eine passende Quelle deshalb ausdrücklich wählen. Solarzellentemperatur, manuell
hinterlegte feste Temperaturen und externe MQTT-Werte sind keine Eingabequellen.
Die Auswahl gilt für alle lokalen Barometer des Geräts.

1. Standortdaten und Temperaturauswahl jeweils speichern.
2. Ausstehende Sensoränderungen anwenden oder das Gerät neu starten.
3. Eine Messperiode abwarten und **Messwerte (Measurements)** prüfen.

Mit **Keine Temperatur – Standardatmosphäre** wird die bisherige Standardformel
verwendet. Auch bei fehlender, ungültiger oder zu alter Temperatur greift diese
Formel. Es wird kein alter Temperaturwert unbegrenzt weiterverwendet.
Beim Start kann der erste Druckwert noch die Standardformel verwenden, bis ein
aktueller Temperaturwert vorliegt. Eine Temperaturänderung fließt mit der nächsten
Druckmessung ein. Vor dem Deaktivieren oder einem inkompatiblen Austausch des
gewählten Sensors zuerst dessen Auswahl als Temperaturquelle entfernen.

## Abgeleiteter Wert und Qualität

`derived: true` kennzeichnet den berechneten Ursprung. Das ist unabhängig von
`quality` und `valid`:

| Eingaben | Ergebnis |
| --- | --- |
| Gültige, aktuelle Temperatur mit Qualität `good` | Temperaturabhängige Berechnung; Qualität des örtlichen Drucks übernehmen |
| Gültige, aktuelle Temperatur mit anderer Qualität | Temperatur verwenden; guter Druck wird `estimated` |
| Keine, fehlende, ungültige oder veraltete Temperatur | Standardformel; guter Druck wird `estimated` |
| Druck bereits `degraded` oder `below_measurement_range` | Eingeschränkte Druckqualität erhalten, niemals aufwerten |
| Fehlende Höhe oder ungültiger örtlicher Druck | Kein gültiger Meereshöhendruck (`valid: false`) |

Eine verwendete simulierte Eingabe kennzeichnet den abgeleiteten Wert ebenfalls
als simuliert; der örtliche Druck behält seine eigene Herkunft.
Die Messwertseite zeigt die verwendete Methode, Temperaturquelle, das bei der
Berechnung festgestellte Alter und den Rückfall- beziehungsweise Qualitätsgrund.

## MQTT, Home Assistant und Display

Die beiden Druckwerte haben getrennte Topics:

```text
envnode/<device>/sensor/<SensorId>/atmospheric_pressure
envnode/<device>/sensor/<SensorId>/sea_level_pressure
```

MQTT und die Messwertseite verwenden für beide Werte die unter **Einheiten (Units)**
gewählte Luftdruckeinheit. Home Assistant Discovery legt für den Meereshöhendruck
einen eigenen Sensor mit ausdrücklich abgeleitetem Namen an.

Im Display-Editor die Quelle `sensor/<SensorId>/sea_level_pressure` auswählen.
Die Property liefert wie der bestehende örtliche Druck den kanonischen Wert in
**Pascal**; ein passender Text ist etwa `NN %.0f Pa`. Vorhandene Displayseiten
werden nicht automatisch umgestellt.

## Berechnung und Plausibilitätsprüfung

Ohne nutzbare Temperatur gilt die Standardatmosphäre:

```text
pNN = p / (1 - h / 44330)^5.255
```

Mit aktueller lokaler Temperatur wird eine Näherung für trockene Luft mit
konstantem Temperaturgradienten verwendet:

```text
pNN = p * (1 + 0.0065*h/(T + 273.15))^(9.80665/(287.05*0.0065))
```

`p` und `pNN` sind Pascal, `h` ist die Sensorhöhe in Metern und `T` die lokale
Außentemperatur in Celsius. Luftfeuchte und ein tatsächlich gemessenes vertikales
Temperaturprofil werden nicht berücksichtigt.

**Beispiel:** 950,3 hPa bei 694 m ergeben mit der Standardformel etwa
**1032,46 hPa**; mit 15,2 °C etwa **1031,08 hPa**. Eine andere Wetterstation
zeigte 1031,03 hPa. Diese Nähe ist ein Plausibilitätscheck, keine Kalibrierung
und kein Sollwert für andere Messsituationen.

## Speicherung und Sicherungen

Standort, Temperaturquelle und Maximalalter werden dauerhaft gespeichert und
überstehen Neustart und Firmware-Update. Backup und Wiederherstellung enthalten
die Einstellungen; die Vorschau zeigt sie vor dem Übernehmen.
Sicherungen der Formate 1 und 2 bleiben lesbar: Sie wählen keine Temperaturquelle.
Format 1 enthält außerdem keinen Standort; Format 2 erhält die Standortangaben.
Aktuelle Sicherungen verwenden Format 3. Ein Factory Reset löscht diese Einstellungen.

[MQTT und Integration](mqtt.md) · [Displays einrichten](values-displays.md) ·
[Sicherung und Diagnose](maintenance.md)

Technische Details und Formelquelle:
[SeaLevelPressure.md im Repository](https://github.com/magicamun/EnvNode/blob/main/docs/SeaLevelPressure.md).

Lizenz: LicenseRef-EnvNode-Use-and-Redistribution-1.0; siehe
[Lizenz und Nutzung](../license.md).
