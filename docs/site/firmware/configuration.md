# Einrichtung und Konfiguration

## Netzwerk und Gerät

In **Network** Hostname, WLAN-SSID und Passwort eintragen.
Unter **Device**, **Time** und **Units** die Geräteangaben, Zeit- und
Darstellungseinstellungen prüfen. Nach einem Netzwerkwechsel die neue
Geräte-IP im Router oder über Serial ermitteln.

Die Konfiguration wird im NVS gespeichert und übersteht reguläre Neustarts
und Firmwareupdates. Factory Reset löscht sie.

## Board und Module

Unter **Diagnostics** Board Identity, aktives Boardprofil und I²C-Ressourcen prüfen.
Ein erkanntes Modul aktiviert keinen Sensor- oder Aktortreiber automatisch.
Die Belegung muss zum realen Mainboard und Slot passen.

## Sensoren

Unter **Sensors** einen Sensorplatz konfigurieren:

1. Physische oder simulierte Implementierung wählen.
2. Bei I²C den Bus und die vom Treiber unterstützte Adresse eintragen.
3. Bei GPIO-Sensoren den passenden freien Anschluss wählen.
4. Speichern und unter **Measurements** die erzeugten Werte prüfen.

[I²C-Treiber und Anschlusshinweise](../sensors/i2c.md)

Beim Kippwaagen-Treiber sind GPIO, Millimeter pro Kippereignis und Entprellung
konfigurierbar. AM2302/DHT22 benötigt einen geeigneten GPIO für sein eigenes
Einleitungsprotokoll; er ist kein I²C-Sensor.

## Aktoren und Controller

Unter **Actuators** GPIO-On/Off oder GPIO-PWM/Level passend zur Hardware konfigurieren.
Aktive Pegel und sicheren Aus-Zustand vor Anschluss einer realen Last prüfen.
[DuoRelay](../modules/duo-relay/index.md) benötigt je Kanal einen passenden On/Off-Aktor.

Unter **Controllers** lokale Funktionen konfigurieren. Blink schaltet zeitgesteuert;
Threshold verarbeitet einen typisierten Messwert mit Hysterese und Frischeprüfung.
Selector ermöglicht eine konfigurierbare Auswahlsteuerung.
Freigegebene Controller-Ziele dürfen nicht gleichzeitig widersprüchlich belegt werden.
Status, Entscheidung und Diagnose vor dem realen Betrieb prüfen.

## Values und Anzeige

**Values** stellt eigene konfigurierbare Werte bereit.
[Displays](../displays/index.md) beziehen ihre Inhalte aus ausgewählten Eigenschaften.
Die angezeigte Vorschau hilft beim Zuordnen und Formatieren der Quellen.

## Abschließende Prüfung

Zunächst lokal korrekte Werte und Schaltzustände prüfen, dann MQTT einrichten.
Einen Neustart durchführen und kontrollieren, ob Konfiguration und Verhalten erhalten bleiben.
