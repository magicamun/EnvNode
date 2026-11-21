# Erste Verbindung und Grundeinrichtung

Diese Seite führt durch die erste Einrichtung eines elektrisch geprüften EnvNode.
Die einzelnen Einstellungsseiten werden in den verlinkten Kapiteln erklärt.
Zum Selbstaufbau beginne mit der passenden [Hardwareanleitung](../hardware/index.md).

## 1. Aufbau und Firmware prüfen

Mainboard nach seiner Hardwareanleitung prüfen und zunächst ohne zusätzliche
Lasten starten. USB-C am ORing-Power-Modul liefert nur Strom. Zum Flashen wird
der USB-Datenanschluss des ESP32-DevKit verwendet.

Falls noch keine passende Firmware installiert ist, folge
[Bauen und Aufspielen](installation.md). Die [Downloads](downloads.md) nennen
die unterstützte Hardware und den jeweiligen Firmwarestand.

## 2. Weboberfläche öffnen

Ohne eingerichtetes WLAN startet ein Setup-Access-Point mit dem Namen
`<Hostname>-Setup`. Verbinde deinen Rechner damit und öffne
**http://192.168.4.1/** im Browser.

Dieser Modus wird auch bei gespeicherten Zugangsdaten erreicht, wenn der
WLAN-Verbindungsversuch beim Start zu lange erfolglos bleibt. Die gespeicherten
Einstellungen bleiben erhalten. Der Setup-Access-Point wird ohne WLAN-Passwort
gestartet; führe die Einrichtung in einer kontrollierten Umgebung durch.

## 3. Netzwerk einrichten

Richte unter **Netzwerk** die Verbindung zu deinem WLAN ein. Für den Einstieg
verwende DHCP. Nach dem Wechsel ist EnvNode unter der Adresse deines eigenen
Netzes erreichbar; die bisherige Browserverbindung kann dabei abbrechen.

[Netzwerkfelder, Passwortänderung und Verbindung prüfen](network.md).

## 4. Gerät, Sprache und Zeit

Die Grundeinstellungen haben jeweils eine eigene Aufgabe:

| Einstellungsseite | Was du hier festlegst |
| --- | --- |
| [Sprache & Zeit](language-time.md) | Oberflächensprache, Zahlenformat, Zeitzone und Zeitserver |
| [Gerät und Standort](device.md) | Gerätename sowie optionale Koordinaten und Sensorhöhe |
| [Einheiten](units.md) | Gewünschte Darstellung der Messgrößen |

Wähle die Sprache vor der Eingabe von Dezimalzahlen. Standortangaben können
zunächst leer bleiben. Wenn du anschließend ein Barometer einrichtest, ergänzt
[Luftdruck auf Meereshöhe](geo-position-pressure.md) die messungsbezogenen Einstellungen.

## 5. Hardware zuordnen

Unter **Hardware** prüfen, ob erkanntes Mainboard, Revision und Modulslots zum
realen Aufbau passen. Eine bereits gültige Identität muss nicht neu geschrieben
werden. Bei leerem oder fehlerhaftem EEPROM führt
[Hardwareerkennung und Provisioning](provisioning.md) durch die Auswahl und Prüfung.

Erst mit passendem Profil die Sensor- und Aktorressourcen auswählen.

## 6. Grundfunktion prüfen

- Status zeigt das erwartete Gerät und die Netzwerkverbindung.
- Sprache und Ortszeit passen; die Zeitsynchronisierung ist erfolgt oder ihr Ausbleiben erklärbar.
- Aktives Boardprofil passt zum aufgebauten Mainboard.
- Keine unerklärten Fehler in den Protokollen.

Richte danach [Sensoren](sensors.md), [Aktoren](actuators.md) und bei Bedarf
[Controller](controllers.md) ein. Sichere den funktionierenden Stand unter
[Konfiguration](maintenance.md#konfiguration-sichern-und-wiederherstellen).
