# Begriffe und Oberfläche

## Was arbeitet zusammen?

| Begriff | Bedeutung | Beispiel |
| --- | --- | --- |
| Mainboard | ESP32 und verfügbare Anschlüsse | Mini oder Weather |
| Modul | Erweiterungsplatine im Slot | DuoRelay |
| Sensor | Firmware-Komponente, die Messungen erzeugt | SHT4x |
| Messwert | Messgröße eines Sensors | Temperatur |
| Aktor | Bedienbarer Ausgang mit einer Fähigkeit | Relais als On/Off-Aktor |
| Controller | Lokale Steuerung eines Aktors | Blink oder Threshold |
| Value | Eigener typisierter Wert | Betriebsart oder Schalterwert |
| Property / Eigenschaft | Lesbarer Zustand einer Komponente | Temperatur oder Controller-Entscheidung |

Ein Modul ist nicht automatisch ein Sensor. DuoRelay wird beispielsweise über
zwei Aktoren bedient. Ein Controller verwendet deren Fähigkeiten und steuert
nicht unmittelbar die physische Verdrahtung.

## Seiten der Weboberfläche

| Seite | Verwendung |
| --- | --- |
| Status | Überblick über Gerät, Netzwerk und Laufzeit |
| Sensoren (Sensors) | Sensorplätze und Implementierungen konfigurieren |
| Messwerte (Measurements) | Messungen und deren Zustand prüfen |
| Aktoren (Actuators) | Ausgänge konfigurieren und manuell bedienen |
| Controller (Controllers) | Lokale Steuerung konfigurieren und starten/stoppen |
| Values | Eigene Werte definieren und ändern |
| Display | Ausgabehardware und Textseiten einrichten |
| Netzwerk (Network) | WLAN, Hostname und IP-Konfiguration |
| MQTT | Broker und Discovery |
| Sprache/Zeit (Locale & Time) | Oberflächensprache, Zeitzone und NTP |
| Einheiten (Units) | Darstellungseinheiten auswählen |
| Gerät (Device) | Geräteangaben |
| Konfiguration (Configuration) | Sicherung und Wiederherstellung |
| Hardware | Board Identity, Module und Provisioning |
| Diagnose (Diagnostics) | Ressourcen und I²C-Scan |
| Logs | Ereignisse und Fehler |
| Firmware | Build-Identität, Update, Neustart und Factory Reset |

## Speichern ist nicht immer Anwenden

Speichern legt eine Konfiguration dauerhaft ab. Bei Sensoren, Aktoren und
Controllern kann die Oberfläche danach noch ausstehende Änderungen anzeigen.
Betätige dann die jeweilige Funktion zum **Anwenden der Änderungen** und prüfe
den aktiven Zustand. Ein gespeicherter Eintrag allein belegt nicht, dass bereits
der neue Treiber oder Controller läuft.

**Start/Stop** betrifft den laufenden Controller; es ist nicht dasselbe wie
seine dauerhaft gespeicherte Aktivierung. Ein Neustart lädt die gespeicherte
Konfiguration erneut. Prüfe deshalb auch das Verhalten nach Neustart.
