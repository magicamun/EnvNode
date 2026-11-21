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
| Value | Eigener Auswahlwert (Enum) | Betriebsart mit festen Optionen |
| Property / Eigenschaft | Lesbarer Zustand einer Komponente | Temperatur oder Controller-Entscheidung |

Ein Modul ist nicht automatisch ein Sensor. DuoRelay wird beispielsweise über
zwei Aktoren bedient. Ein Controller verwendet deren Fähigkeiten und steuert
nicht unmittelbar die physische Verdrahtung.

## Seiten der Weboberfläche

| Seite | Verwendung |
| --- | --- |
| [Status](maintenance.md#diagnose) | Überblick über Gerät, Netzwerk und Laufzeit |
| [Sensoren](sensors.md) | Sensorplätze und Implementierungen konfigurieren |
| [Messwerte](sensors.md#messwerte-beurteilen) | Messungen und deren Zustand prüfen |
| [Aktoren](actuators.md) | Ausgänge konfigurieren und manuell bedienen |
| [Controller](controllers.md) | Lokale Steuerung konfigurieren und starten/stoppen |
| [Werte (Values)](values.md) | Eigene Auswahlwerte definieren und ändern |
| [Anzeige](values-displays.md) | Ausgabehardware und Textseiten einrichten |
| [Netzwerk](network.md) | WLAN, Hostname und IP-Konfiguration |
| [MQTT](mqtt.md) | Broker und Discovery |
| [Sprache & Zeit](language-time.md) | Oberflächensprache, Zeitzone und NTP |
| [Einheiten](units.md) | Darstellungseinheiten auswählen |
| [Gerät](device.md) | Geräteangaben und [Standort](device.md#standort-eingeben) |
| [Konfiguration](maintenance.md#konfiguration-sichern-und-wiederherstellen) | Sicherung und Wiederherstellung |
| [Hardware](provisioning.md) | Board Identity, Module und Provisioning |
| [Diagnose](maintenance.md#diagnose) | Ressourcen und I²C-Scan |
| [Netzwerklogging](network-logging.md) | Meldungen und erweiterte Diagnosen an einen externen Empfänger senden |
| [Protokolle](maintenance.md#diagnose) | Lokale Ereignisse und Fehler |
| [Firmware](maintenance.md#web-ota) | Build-Identität, Update, Neustart und Factory Reset |

Die Links führen zur jeweiligen Erklärung in dieser Dokumentation.

## Konfiguration, Laufzeit und Anzeige

Eine **Konfiguration** beschreibt, was EnvNode tun soll: etwa Sensortyp,
Messintervall und Controllerparameter. Der **Laufzeitzustand** beschreibt, was
gerade passiert: letzter Messwert, eingeschalteter Ausgang oder laufender
Controller. Die Oberfläche zeigt beides; ein gespeicherter Eintrag bestätigt
noch keine erfolgreiche Messung oder Schaltung.

Die Platznummer identifiziert eine Komponente. Ein eigener Name macht ihren Zweck
verständlich. Benenne beispielsweise Gehäuse- und Außensensor unterschiedlich;
beide können dieselbe Implementierung verwenden. Ein BME280 belegt einen
Sensorplatz und liefert mehrere Messgrößen. Er benötigt keinen eigenen Platz
für jede Messgröße.

Quellen und Ziele werden ausdrücklich zugeordnet: Ein Sensor liefert Messungen,
ein Controller verwendet eine Quelle und gegebenenfalls einen Aktor als Ziel.
Eine Displayquelle liest eine Eigenschaft, ohne die Komponente zu steuern.
Änderst du eine Implementierung oder entfernst eine Komponente, prüfe deshalb
auch ihre Verwendung in Controllern, Druckberechnung und Displays.

## Speichern ist nicht immer Anwenden

Speichern legt eine Konfiguration dauerhaft ab. Bei Sensoren, Aktoren und
Controllern kann die Oberfläche danach noch ausstehende Änderungen anzeigen.
Betätige dann die jeweilige Funktion zum **Anwenden der Änderungen** und prüfe
den aktiven Zustand. Ein gespeicherter Eintrag allein belegt nicht, dass bereits
der neue Treiber oder Controller läuft.

### Welche Aktion bewirkt was?

| Aktion | Wirkung | Nach einem Neustart |
| --- | --- | --- |
| Formular bearbeiten | Noch keine Änderung am Gerät, solange nicht gespeichert wurde | Nicht gespeicherte Eingaben gehen verloren |
| Sensor-, Aktor- oder Controllerkonfiguration speichern | Einstellungen dauerhaft ablegen; eine Laufzeitübernahme kann noch ausstehen | Gespeicherte Einstellungen werden geladen |
| Ausstehende Änderungen anwenden | Den betroffenen Laufzeitbereich mit gespeicherten Einstellungen neu aufbauen | Die Konfiguration bleibt erhalten |
| Controller Start/Stop | Laufenden Controller bedienen | Die gespeicherte Aktivierung bestimmt den erneuten Start |
| Aktor Ein/Aus oder Pegel setzen | Ausgang im laufenden Betrieb ändern | Der Handbefehl wird nicht als gewünschter Dauerzustand gespeichert |
| Value setzen | Auswahl sofort ändern; Speicherung hängt von der Neustartregel des Values ab | Siehe [Values](values.md#verhalten-nach-neustart) |
| Displayvorschau | Eingaben mit verfügbaren Laufzeitwerten prüfen | Noch keine Speicherung der bearbeiteten Seite |
| Gerät neu starten | Alle Laufzeitbereiche neu initialisieren | Gespeicherte Konfiguration bleibt erhalten |

Anwenden kann Messungen unterbrechen und Controller beziehungsweise Ausgänge
neu initialisieren. Nach jeder Übernahme Zuordnung und Zustand kontrollieren.
Bei mehreren ausstehenden Änderungen die Rückmeldung der Oberfläche abarbeiten,
bis keine weitere Übernahme verlangt wird. MQTT-, Netzwerk- und Zeiteinstellungen
können eine Dienst- oder Geräteneustart-Aktion verlangen; die Rückmeldung nennt
den erforderlichen Schritt.

Ein **Factory Reset** löscht dagegen die gespeicherte Konfiguration; er gehört
nicht zur normalen Übernahme von Einstellungen.

### Aktivierung und Start/Stop

**Start/Stop** betrifft den laufenden Controller; es ist nicht dasselbe wie
seine dauerhaft gespeicherte Aktivierung. Ein Neustart lädt die gespeicherte
Konfiguration erneut. Prüfe deshalb auch das Verhalten nach Neustart.
