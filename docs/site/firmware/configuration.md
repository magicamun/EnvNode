# Erste Inbetriebnahme

## 1. Aufbau und Firmware prüfen

Mainboard nach seiner Hardwareanleitung prüfen und zunächst ohne zusätzliche
Lasten starten. USB-C am ORing-Power-Modul liefert nur Strom. Zum Flashen wird
der USB-Datenanschluss des ESP32-DevKit verwendet.

## 2. Weboberfläche öffnen

Ohne eingerichtetes WLAN startet ein Setup-Access-Point mit dem Namen
`<Hostname>-Setup`. Verbinde deinen Rechner damit und öffne
**http://192.168.4.1/** im Browser.

Dieser Modus wird auch bei gespeicherten Zugangsdaten erreicht, wenn der
WLAN-Verbindungsversuch beim Start zu lange erfolglos bleibt. Prüfe dann
Router-Erreichbarkeit, SSID und Passwort. Die gespeicherten Einstellungen
bleiben erhalten. Nach einer erfolgreichen WLAN-Verbindung gilt die dort
zugewiesene IP-Adresse.

Der Setup-Access-Point wird im aktuellen Code ohne WLAN-Passwort gestartet;
führe die Einrichtung in einer kontrollierten Umgebung durch.

## 3. Netzwerk einrichten

Unter **Netzwerk (Network)** Hostname, WLAN-SSID und Passwort setzen.
Für den ersten Betrieb DHCP verwenden. Eine statische Adresse benötigt zusätzlich
passende IP, Netzmaske, Gateway und DNS; falsche Angaben können die Erreichbarkeit verhindern.

Speichern und die Rückmeldung beachten. Nach dem Wechsel zurück ins normale WLAN
die neue IP im Router oder in der seriellen Ausgabe ermitteln.
Das Abbrechen der bisherigen Verbindung während eines Netzwerkwechsels ist erwartbar.

## 4. Gerät, Sprache und Zeit

Unter **Gerät (Device)** die Geräteangaben prüfen. Unter **Sprache/Zeit** die
gewünschte Sprache, Zeitzone und NTP-Server einstellen. Synchronisierung kontrollieren.
Unter **Einheiten (Units)** die gewünschten Darstellungseinheiten auswählen;
diese Einstellungen können auch zukünftige MQTT-Ausgaben beeinflussen.

Die Zeitzonenauswahl umfasst weltweit alle 597 IANA-Bezeichner (Stand 2026e),
einschließlich Aliasnamen. Gespeichert wird die zugehörige POSIX-Regel; nach dem
Speichern kann deshalb ein gleichbedeutender Ortsname erscheinen. Unregelmäßige
Zeitumstellungen wie die Ramadan-Ausnahmen in Marokko sind mit diesem Zeitmodell
nicht vollständig abbildbar. Details und Aktualisierung:
[Zeitzonenkonfiguration](../../TimezoneConfiguration.md).

## 5. Hardware zuordnen

[Hardware und Provisioning](provisioning.md) öffnen. Erst nach Kontrolle des
aktiven Boardprofils die Sensor- und Aktorressourcen auswählen.
Ein Diagnosehinweis auf einen Entwicklungs-Fallback ist kein Nachweis einer
korrekt provisionierten Board Identity.

## 6. Grundfunktion prüfen

- Status zeigt das erwartete Gerät und die Netzwerkverbindung.
- Zeit ist synchronisiert oder ihre noch laufende Synchronisierung ist erklärbar.
- Aktives Boardprofil passt zum aufgebauten Mainboard.
- Keine unerklärten Fehler in den Logs.

[Weiter zur ersten Messung](sensors.md)
