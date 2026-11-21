# Gerät und Standort

Unter **Gerät** stehen der eigene Gerätename, Standortangaben und ein Verweis
auf die erkannte Hardware. Die Einstellungen für die
[Luftdruckberechnung](geo-position-pressure.md) befinden sich ebenfalls auf dieser
Webseite, werden aber bei der zugehörigen Messfunktion erklärt.

## Gerätename

Der **Gerätename** bezeichnet diesen EnvNode in der Oberfläche und trägt zur
MQTT-Identität und den Topics bei. Er darf bis zu 32 UTF-8-Bytes lang sein.
Wähle ihn eindeutig und möglichst früh, bevor
du externe Anzeigen oder Automationen anbindest.

| Bezeichnung | Wo festgelegt | Aufgabe |
| --- | --- | --- |
| Gerätename | Gerät | Bezeichnung in der Anwendung und Grundlage der MQTT-Gerätekennung |
| Hostname | Netzwerk | Name des Geräts im Netzwerk |
| Boardprofil und Revision | Hardware / EEPROM | Beschreiben die tatsächliche Platine und ihre Anschlüsse |

Eine Namensänderung ist weder ein Wechsel des Boardprofils noch eine Änderung
der physischen Messstelle. Nach **Geräteeinstellungen speichern** die geforderte
Laufzeitaktion für MQTT ausführen und verwendete Topics beziehungsweise externe
Zuordnungen kontrollieren. Der Gerätename bleibt nach Neustart erhalten.

## Standort eingeben

Im Abschnitt **Standort (optional)** die bekannten Angaben eintragen:

| Feld | Format und Bereich | Bedeutung |
| --- | --- | --- |
| Latitude / Breitengrad | Dezimalgrad, −90 bis +90 | Nord positiv, Süd negativ |
| Longitude / Längengrad | Dezimalgrad, −180 bis +180 | Ost positiv, West negativ |
| Sensorhöhe | Meter, −1000 bis +10000 | Tatsächliche Höhe des Sensors über Meereshöhe, einschließlich Gebäude- oder Montagehöhe |

Die Felder sind unabhängig optional. Unbekannte Angaben leer lassen:
**0 m ist eine gültige Höhe**, kein Platzhalter für „unbekannt“. Die Sensorhöhe
ist nicht nur der Abstand zwischen Gehäuse und Boden.

Die Eingabe verwendet das Zahlenformat der gewählten [Sprache](language-time.md):
Deutsch mit Komma, etwa `48,137154`; Englisch mit Punkt, etwa `48.137154`.
Keine Tausendertrennzeichen und keine Grad/Minuten/Sekunden-Schreibweise verwenden.
Die internen Werte und Sicherungen bleiben von der Schreibweise unabhängig.

## Welche Funktion braucht welche Angabe?

| Funktion | Benötigte Standortangaben |
| --- | --- |
| Normale Temperatur- und Feuchtemessung | Keine |
| Örtlicher Luftdruck des Barometers | Keine |
| Berechneter Luftdruck auf Meereshöhe | Sensorhöhe; zusätzlich kann eine lokale Außentemperatur verwendet werden |
| Solar Position | Breitengrad, Längengrad und synchronisierte Zeit |

Latitude und Longitude gehen nicht in die Druckreduktion ein. Ohne Sensorhöhe
bleibt der zusätzliche Meereshöhendruck ungültig; die örtliche Druckmessung
bleibt davon unberührt.

## Speichern und prüfen

Gerätename und Standort besitzen getrennte Formulare. Den jeweiligen Abschnitt
speichern; eine Schaltfläche speichert nicht automatisch alle anderen Abschnitte
der Webseite mit. Nach Standortänderungen die angeforderte Sensorübernahme
ausführen oder das Gerät neu starten.

Die Standortangaben werden dauerhaft gespeichert und sind Bestandteil der
[Konfigurationssicherung](maintenance.md#konfiguration-sichern-und-wiederherstellen).
Nach erneutem Öffnen beziehungsweise Neustart die gespeicherten Angaben prüfen.
Eine konfigurierte Höhe wird erst durch eine aktive Barometermessung im
Meereshöhendruck sichtbar.

Der Abschnitt **Hardware** auf der Geräteseite zeigt eine Zusammenfassung.
Identifikation und Änderung der EEPROM-Daten erfolgen unter
[Hardwareerkennung und Provisioning](provisioning.md).
