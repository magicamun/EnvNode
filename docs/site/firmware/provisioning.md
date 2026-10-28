# Hardware identifizieren und provisionieren

Provisioning schreibt die Identität einer Baugruppe in deren EEPROM.
Es ist bei leerem oder fehlerhaftem Datensatz nötig, nicht bei jedem Neustart.

| Speicher | Aufgabe |
| --- | --- |
| Mainboard-EEPROM | Board Identity und Auswahl des passenden Boardprofils |
| Modul-EEPROM | Module Identity beziehungsweise Descriptor und Kompatibilitätsprüfung |
| ESP32-NVS | WLAN, MQTT und Anwendungs-Konfiguration |

## Erkennung zuerst prüfen

Unter **Hardware** erkannte Identität und aktives Profil kontrollieren;
unter **Diagnose** gegebenenfalls I²C scannen. Ein nicht erreichbares EEPROM
zuerst auf Bestückung, Versorgung und Verbindung prüfen. Ein I²C-Antwortsignal
bestätigt noch keinen gültigen Datensatz.

| EEPROM | Bus | Adresse |
| --- | --- | --- |
| Mainboard | I2C0 | 0x50 |
| Modul Slot A | I2C0 | 0x52 |
| Modul Slot B, wenn vorhanden | I2C0 | 0x53 |

## Unterstützte Mainboard-Revisionen

Ab **EnvNode 0.8.2** und **Testboard 0.7.0** sind diese Identitäten auswählbar:

| Board | Hardware-Identität | Beispiel-PCB |
| --- | --- | --- |
| EnvNode Mainboard | 0.3 | ältere Mainboard-Ausführung |
| EnvNode Weather | 0.2 oder 0.3 | 0.2.1 bzw. 0.3.0 |
| EnvNode Mini | 0.8 oder 0.9 | 0.8.1 bzw. 0.9.0 |

Die tatsächliche Revision auswählen. Vorhandene EEPROM-Daten werden durch ein
Firmwareupdate weder migriert noch automatisch auf eine neue Hardware-Revision
umgeschrieben. Die normale Firmware verwendet nach dem Neustart die gespeicherte
Revision und deren Ressourcen.

## Mainboard schreiben

1. Mainboard und reale PCB-Revision anhand der Platine feststellen.
2. Unter **Hardware** das passende angebotene Boardprofil auswählen.
3. Die Seriennummer eintragen. Erlaubt sind bis zu 17 Buchstaben, Ziffern sowie
   `-`, `_` und `.`; ein leeres Feld kennzeichnet eine noch nicht zugewiesene
   Seriennummer.
4. Profil, zugehörige Revision und Seriennummer überprüfen.
5. Die ausdrückliche Schreibbestätigung setzen und den Vorgang ausführen.
6. Nur eine erfolgreiche Schreib- und Rückleseprüfung als Erfolg werten.
7. Neustarten und aktives Boardprofil erneut prüfen.

Die Revision wird durch das gewählte unterstützte Profil bestimmt. Keine
ähnliche oder höhere Version als Ersatz für ein fehlendes Profil wählen.
Das Schreiben ändert das aktive Boardprofil des laufenden Boots nicht;
der Neustart ist deshalb Bestandteil des Ablaufs.

## Modul schreiben

Die aktuelle Webfunktion bietet die Descriptor-Vorlage **DuoRelay 0.3**.
Slot A oder B passend zum eingesetzten Modul wählen, die angebotenen
Fertigungsangaben prüfen und die ausdrückliche Bestätigung verwenden.
Ergebnis und die anschließend erkannte Identität kontrollieren.
Andere Module werden nicht durch Auswahl der DuoRelay-Vorlage provisioniert.

Ein leeres EEPROM muss mit einem passenden Descriptor provisioniert
werden. Die hier dokumentierte Webfunktion bietet noch keine passende Vorlage
für AnalogHydroPressure; dafür niemals die DuoRelay-Vorlage verwenden.

## Was danach noch nötig ist

Identifikation erzeugt nicht automatisch eine fertige Anwendung.
Sensoren, Aktoren und Controller separat konfigurieren und ihre Änderungen anwenden.
Die ADS1115-Adresse am AnalogHydroPressure ist unabhängig von der Identity-Adresse;
dessen Buswahljumper betreffen den ADC, nicht das Identity-EEPROM.

[Mini](../mainboards/mini/index.md) · [Weather](../mainboards/weatherstation/index.md)
· [DuoRelay](../modules/duo-relay/index.md)

## Weather Revision 0.2: Provisionierung

Dieser Ablauf gilt für das aufgebaute Weather 0.2; PCB 0.2.1 verwendet dieselbe
Board Identity 0.2. Voraussetzung ist eine Firmware, die unter **Hardware** das
Profil **EnvNode Weather**, Revision **0.2**, anbietet. Fehlt dieses Angebot,
zuerst eine Firmware mit Weather-Profil bauen und installieren. Das ältere
**EnvNode Mainboard 0.3** ist kein Ersatz: dessen Slot-A-GPIO1 ist GPIO4 statt GPIO33.

### 1. Mainboard ohne Modul provisionieren

1. Ohne Relais-Modul und RainDetector starten. Firmwareversion und Ausgangszustand
   protokollieren. Bei leerem EEPROM ist `NotProvisioned` zu erwarten; das aktive
   Profil kann bis zur Provisionierung aus `BuildFallback` stammen.
2. I2C0 prüfen: das Board-EEPROM muss bei `0x50` antworten.
3. Unter **Hardware** **EnvNode Weather / 0.2** auswählen. Eine für dieses Board
   vergebene, dauerhaft dokumentierte alphanumerische Seriennummer eintragen.
   Ein leeres Feld bedeutet noch nicht zugewiesen (`UnassignedSerial`).
4. Schreibbestätigung setzen, provisionieren und erfolgreiche Rückleseprüfung abwarten.
5. Neustarten. Erwartung: Status `Valid`, Quelle `EEPROM`, Profil **EnvNode Weather**,
   Revision **0.2**, eigene Seriennummer, genau **Slot A**.
6. Aus- und wieder einschalten und dieselben Angaben erneut prüfen.

Erst nach diesen Prüfungen weitere Hardware anschließen. WLAN/MQTT-Konfiguration
in NVS und Board Identity im EEPROM sind getrennte Datenbestände.

## Mini Revision 0.8: Provisionierung

Mini-PCBs `0.8.x` verwenden dasselbe Firmwareprofil und dieselbe Board Identity
**EnvNode Mini 0.8**. Der Patchstand wird nicht im EEPROM gespeichert. Unter
**Hardware** das Profil **EnvNode Mini / 0.8** auswählen, eine Seriennummer
eintragen, schreiben und rücklesen lassen und anschließend neu starten. Danach
müssen Quelle `EEPROM`, Status `Valid`, Profil **EnvNode Mini**, Revision `0.8`
und zwei Modulslots angezeigt werden.

## Separate Tester-Firmware

Das PlatformIO-Ziel `testboard` ist eine eigene Anwendung mit eigener
Konfiguration. Sein Menü **Provisioning** schreibt ein extern angeschlossenes
Mainboard-EEPROM; dafür ist **kein Neustart des Testers** erforderlich. Ein leeres
oder nicht erkanntes EEPROM setzt keine Revision voraus: Board und Revision
müssen ausdrücklich gewählt werden. Die passende Boardansicht dient zur Kontrolle.

Der separate Bus verwendet **SDA GPIO16 / SCL GPIO5**, das Board-EEPROM Adresse
`0x50`. ESP32 des Prüflings entfernen und andere Versorgungen, Mess-Pigtails und
Input-Adapter trennen, bevor der Provisioning-Stecker angeschlossen wird. Das
Kabel vor einer passiven Durchgangsprüfung wieder abziehen; die Firmware schaltet
dessen Versorgung nicht ab.

Die **Anschlussprüfung ist derzeit eine Simulation**. Das Provisioning greift
dagegen auf das echte EEPROM zu und muss mit der fertigen Tester-Hardware noch
physisch validiert werden. Eine laufende Anschlussprüfung sperrt Provisioning.
Modul-Provisioning ist in der Tester-Anwendung noch nicht freigeschaltet.

Die Prüfprofile und Boardbilder werden über docs.envnode.de bereitgestellt.
Eingebaute Profile bleiben ohne Netz verfügbar. Die unabhängige Signaturprüfung
ist noch nicht implementiert; der aktuelle Abruf nutzt geprüftes HTTPS und
SHA-256-Inhaltsprüfungen.
