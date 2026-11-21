# Sprache und Zeit

Die Weboberfläche fasst diese Einstellungen unter **Sprache & Zeit** zusammen.
Sprache, Zeitzone und Uhrzeitsynchronisierung erfüllen unterschiedliche Aufgaben.

## Einstellungen

| Feld | Bedeutung |
| --- | --- |
| Gebietsschema / Locale | Deutsch oder Englisch; bestimmt Oberflächensprache sowie Datums- und Zahlenformat. |
| Zeitzone | Regel für die lokale Uhrzeit einschließlich der vorgesehenen Sommerzeitumschaltung. |
| NTP-Server 1 | Erreichbarer Zeitserver, von dem EnvNode seine Uhrzeit bezieht. |
| NTP-Server 2 | Zweiter Zeitserver als weitere Quelle. |

Die Sprache bestimmt nicht automatisch den Standort oder die Zeitzone. Wähle
die passende Zeitzone ausdrücklich. Eigene Sensor- und Gerätenamen sowie selbst
eingetragene Displaytexte werden beim Sprachwechsel nicht übersetzt.

Deutsche Dezimaleingaben verwenden bei Standort und Temperatur-Maximalalter ein
Komma, englische einen Punkt. Interne Daten, MQTT und Sicherungsdateien werden
dadurch nicht in ein sprachabhängiges Format umgeschrieben.

## Speichern und Uhrzeit kontrollieren

1. Gebietsschema, Zeitzone und erreichbare Zeitserver auswählen.
2. **Sprache und Zeit speichern** und die Rückmeldung prüfen.
3. Bei geänderten Zeitparametern die angeforderte Zeitdienst-Aktion ausführen.
   Eine reine Sprachänderung benötigt keinen Neustart des gesamten Geräts.
4. Auf dieser Seite oder unter **Status** den Synchronisationszustand und die
   angezeigte Ortszeit prüfen.

Nach dem Start muss die Zeit zunächst synchronisiert werden. Ein erreichbares
WLAN allein bestätigt keine Verbindung zum Zeitserver. Bei ausbleibender
Synchronisierung DNS, Gateway und Zeitserver prüfen. Bei plausiblen Minuten,
aber falscher Stunde zuerst die Zeitzone kontrollieren.

Die Einstellungen überstehen Neustart und Firmwareupdate. Die laufende Uhrzeit
wird nach dem Start erneut synchronisiert. Zeitangaben in Messungen und
Protokollen erst zusammen mit dem Synchronisationszustand beurteilen.

## Ortsnamen und Zeitregeln

Mehrere Orte können dieselbe Zeitregel verwenden. Nach dem Speichern kann ein
gleichbedeutender Ortsname erscheinen; maßgeblich ist die wirksame lokale Zeit.
Unregelmäßige Sonderregelungen, etwa zeitweilige Abweichungen von der normalen
Sommerzeit, sind mit dem verwendeten Zeitmodell nicht vollständig abbildbar.
Technische Details stehen bei Bedarf in der
[Zeitzonenkonfiguration im Repository](https://github.com/magicamun/EnvNode/blob/main/docs/TimezoneConfiguration.md).

[Gerät und Standort](device.md) · [Einheiten](units.md)
