# Firmware

Fertige Firmware für beide Anwendungen findest du unter [Downloads](downloads.md).

EnvNode hat zwei Firmware-Anwendungen mit gemeinsamer Basis für Netzwerk,
Weboberfläche, Sprache/Zeit, Protokoll und OTA. Welche Anwendung läuft, wird beim
Build festgelegt; das ist keine zur Laufzeit umschaltbare Betriebsart.

| Anwendung | Aufgabe | Einstieg |
| --- | --- | --- |
| **EnvNode** | Sensoren erfassen, Ausgänge und Controller betreiben, Messwerte anzeigen und per MQTT bereitstellen | [EnvNode bedienen](index.md) |
| **Test/Provisioning** | Testhardware in Betrieb nehmen, passive Mainboard-Verbindungen prüfen und externe Board-EEPROMs provisionieren | [Test/Provisioning bedienen](test-provisioning/index.md) |

Die Anwendungen haben eigene Versionsnummern und getrennte Geräteeinstellungen.
Das Aufspielen einer Test-Firmware macht aus einem normalen Mainboard noch keine
Testhardware. Elektrische Messungen brauchen den Test-Controller und den passenden
Input-Adapter.

Der Bereich **EnvNode** beschreibt die normale Gerätefirmware. Der Bereich
**Test/Provisioning** dokumentiert den Entwicklungsstand der Prüfanwendung und
benennt ausdrücklich, welche Funktionen real messen und welche simuliert werden.
