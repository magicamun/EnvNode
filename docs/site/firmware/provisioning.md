# Hardware identifizieren und provisionieren

Die eigenständige Test-Firmware und die Trennung von Messbetrieb und externem
Provisioning beschreibt [Test/Provisioning](test-provisioning/provisioning-profile.md).

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

Für DuoRelay bietet die Webfunktion die Descriptor-Vorlage **DuoRelay 0.3**.
Ein SD-Card-Descriptor kann aus dem Hardwarekatalog geschrieben werden, wird
im klassischen ESP32-Build aber nicht als Laufzeitgerät unterstützt.
Slot A oder B passend zum eingesetzten Modul wählen, die angebotenen
Fertigungsangaben prüfen und die ausdrückliche Bestätigung verwenden.
Ergebnis und die anschließend erkannte Identität kontrollieren.
Andere Module werden nicht durch Auswahl der DuoRelay-Vorlage provisioniert.

Ein leeres EEPROM muss mit einem passenden Descriptor provisioniert
werden. Die hier dokumentierte Webfunktion bietet noch keine passende Vorlage
für AnalogHydroPressure; dafür niemals die DuoRelay-Vorlage verwenden.

## SD-Card 0.1: historischer Betriebsablauf

Die folgenden Schritte beziehen sich auf den früheren SD-Build. Der aktuelle
klassische ESP32 meldet fehlende Treiberunterstützung und bietet keine SD-Seite.
Ein EEPROM-Descriptor allein aktiviert keinen Treiber.

1. Einen historischen SD-fähigen Build von **0.8.7** verwenden.
2. Das stromlos eingesetzte Modul und seinen tatsächlichen Slot A oder B feststellen.
3. Unter **Hardware** den gemeinsamen Hardwarekatalog laden und **EnvNode SD-Card 0.1** (`sd-card-0.1`) auswählen. Kein DuoRelay-Profil als Ersatz verwenden.
4. Slot, Profil und Fertigungsangaben prüfen, Schreiben ausdrücklich bestätigen und erfolgreiche Schreib-/Rückleseprüfung abwarten. Ein bereits gültiges SD-EEPROM muss nicht erneut geschrieben werden.
5. Neu starten. Im Geräteinventar muss `sd.1` als Infrastruktur mit Treiber `sd-spi v1`, Fähigkeit `storage.sd` und Status `Ready` erscheinen.
6. Unter **SD-Karte** Kartenzustand und Dateiliste prüfen. Slot und SD-Zugriff stammen automatisch aus dem Descriptor; Systemlog und Diagnoseaufzeichnung separat einschalten.

Das Profilminimum 0.8.6 prüft nur einen Teil der Kompatibilität. Ältere Builds
dieser Version können das Profil schreiben, aber noch keinen SD-Treiber enthalten.
Die öffentliche integrierte Version ist 0.8.7. Bei `unknown` oder
`UnsupportedDeviceKind` zuerst Firmwarestand und Treiberunterstützung prüfen.
Bei Modulkonflikt oder nicht unterstütztem Vertrag bleibt der Zugriff gesperrt.

[Aufbauanleitung](../modules/sd-card/index.md) · [SD-Bedienung](sd-card.md)

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
5. Neustarten. Erwartung: Status `Valid`, Quelle `PortMap` (bei alten ENID-only-Daten `EEPROM`), Profil **EnvNode Weather**,
   Revision **0.2**, eigene Seriennummer, genau **Slot A**.
6. Aus- und wieder einschalten und dieselben Angaben erneut prüfen.

Erst nach diesen Prüfungen weitere Hardware anschließen. WLAN/MQTT-Konfiguration
in NVS und Board Identity im EEPROM sind getrennte Datenbestände.

## Mini Revision 0.8: Provisionierung

Mini-PCBs `0.8.x` verwenden dasselbe Firmwareprofil und dieselbe Board Identity
**EnvNode Mini 0.8**. Der Patchstand wird nicht im EEPROM gespeichert. Unter
**Hardware** das Profil **EnvNode Mini / 0.8** auswählen, eine Seriennummer
eintragen, schreiben und rücklesen lassen und anschließend neu starten. Danach
müssen Quelle `PortMap` (bei alten ENID-only-Daten `EEPROM`), Status `Valid`, Profil **EnvNode Mini**, Revision `0.8`
und zwei Modulslots angezeigt werden.

## Separate Tester-Firmware

Das PlatformIO-Ziel `testboard` ist eine eigene Anwendung mit eigener
Konfiguration. Sein Menü **Provisioning** schreibt ein extern angeschlossenes
Board- oder Modul-EEPROM; dafür ist **kein Neustart des Testers** erforderlich. Ein leeres
oder nicht erkanntes EEPROM setzt keine Revision voraus: Board und Revision
müssen ausdrücklich gewählt werden. Die passende Boardansicht dient zur Kontrolle.

Der separate Bus verwendet **SDA GPIO16 / SCL GPIO5**, das Board-EEPROM Adresse
`0x50`, die Modul-EEPROMs `0x52` (A) und `0x53` (B). ESP32 des Prüflings entfernen und andere Versorgungen, Mess-Pigtails und
Input-Adapter trennen, bevor der Provisioning-Stecker angeschlossen wird. Das
Kabel vor einer passiven Durchgangsprüfung wieder abziehen; die Firmware schaltet
dessen Versorgung nicht ab.

Die **Anschlussprüfung ist derzeit eine Simulation**. Das Provisioning greift
dagegen auf das echte EEPROM zu und muss mit der fertigen Tester-Hardware noch
physisch validiert werden. Eine laufende Anschlussprüfung sperrt Provisioning.
Für Module steht zunächst die DuoRelay-0.3-Vorlage zur Verfügung. Der Tester
validiert und schreibt denselben ENHD-Datensatz wie die normale Firmware, ohne
den Modulbetrieb zu unterstützen. Fehlende Treiber oder eine höhere erforderliche
Betriebs-Firmwareversion verhindern das Provisioning nicht. Modulschreibvorgänge
sind ebenfalls während einer Anschlussprüfung gesperrt.

Die Prüfprofile und Boardbilder werden über docs.envnode.de bereitgestellt.
Eingebaute Profile bleiben ohne Netz verfügbar. Die unabhängige Signaturprüfung
ist noch nicht implementiert; der aktuelle Abruf nutzt geprüftes HTTPS und
SHA-256-Inhaltsprüfungen.

## Selbstbeschreibende Portzuordnung

Board-Provisioning schreibt zusätzlich zur bisherigen ENID-Identität die
Portzuordnung **ENPM 1**. Das gilt für die normale Firmware und den separaten
Tester. Beim nächsten Start liest die normale Firmware GPIOs, Busse und
Modulslots daraus. Für gültige alte Identitäten ohne ENPM bleiben die eingebauten
Boardtabellen verfügbar; ein Start allein verändert das EEPROM nicht.

Bei beschädigtem ENPM, widersprüchlicher Identität oder nicht erfüllten
Betriebsanforderungen wird keine GPIO-Zuordnung aktiviert. Diagnose und Bootlog
weisen den ENPM-Status aus. Die Migration erfolgt durch erneutes Provisioning.

## Nachladbare Profile ab EnvNode 0.8.3 / Tester 0.8.0

In derselben Provisioning-Ansicht steht zusätzlich **Nachladbare Hardwareprofile**
zur Verfügung. Einmalig werden die verfügbaren Boards und Module aus dem
Dokumentationskatalog geladen; **Profile aktualisieren** wiederholt die Anfrage.
Seit EnvNode 0.8.4 / Tester 0.8.1 übernimmt der Browser HTTPS und die
SHA-256-Prüfung; er lädt die geprüfte Vorlage anschließend lokal zum Gerät hoch.
Der Browser benötigt Internetzugang und eine Verbindung zum Gerät. Die eingebauten Formulare bleiben
auch ohne Internet verwendbar.

Profil, Revision und Board-/Modulbild dienen zur Auswahl. Die angezeigte
Mindestversion gilt für den Betrieb; sie sperrt das Provisioning nicht. Für ein
Modul zusätzlich den EEPROM-Slot A/B wählen und die individuellen
Herstellungsdaten eintragen. Erst die Schreibbestätigung löst Download,
Validierung und EEPROM-Schreiben aus. Ohne passende Treiber wird das Modul nach
dem Neustart angezeigt, reserviert seine Ports und bleibt funktional inaktiv.
Die neue Hardwarekonfiguration wird im normalen EnvNode nach einem Neustart
übernommen; der Tester schreibt weiterhin den externen Prüfling.

Der Katalog enthält zunächst die vorhandenen fünf Boardrevisionen und DuoRelay
0.3. SD und SX1262-Remote folgen mit geprüften Hardwareprofilen. Während einer
laufenden Anschlussprüfung sind auch die neuen Remote-Aktionen gesperrt.

## Gemeinsamer Dialog ab EnvNode 0.8.5 / Tester 0.8.2

**Board** oder **Modul** auswählen, anschließend das konkrete Profil. Das Bild
und die Betriebsmindestversion beziehen sich auf genau diese Auswahl. Passende
Onlineprofile ersetzen eingebaute Listeneinträge; bei fehlendem Internet bleiben
eingebaute Profile im selben Dialog verfügbar. Katalogaktionen und ausführliche
Anschlusshinweise lassen sich aufklappen.

Seriennummer, Produktionscharge und Fertigungsdatum sind für beide Typen optional;
nur beim Modul wird zusätzlich der Slot gewählt. Nach Änderungen erneut das
Schreiben bestätigen und **Board provisionieren** bzw. **Modul provisionieren**
auslösen. Die Ziele bleiben getrennt: externer Prüfling beim Tester, eigene
Hardware beim normalen EnvNode.

Board-Charge und -Datum werden mit der Portzuordnung verifiziert gespeichert.
Diese Erweiterung benötigt im Betrieb mindestens EnvNode 0.8.5, was die Anzeige
während der Eingabe berücksichtigt. Bestehende Datensätze bleiben lesbar. Ohne
Charge/Datum bleibt das bisherige Boardformat erhalten.

## EEPROM-Inhalt prüfen ab EnvNode 0.8.6 / Tester 0.8.3

**Aktuell im EEPROM** zeigt den frisch gelesenen Inhalt des gewählten Boards oder
Modul-Slots einschließlich Lesestatus und Fertigungsdaten. **Neu zu schreiben**
zeigt die geplanten Angaben und markiert Abweichungen in einer Zusammenfassung.
**EEPROM erneut lesen** aktualisiert den Istzustand. Vorhandene Fertigungsdaten
werden nur mit **Vorhandene Fertigungsdaten übernehmen** in die Eingaben kopiert.

Vor dem Schreiben prüft die Firmware den EEPROM-Inhalt erneut. Bei Änderungen
oder Lesefehlern wird nichts geschrieben; Eingaben bleiben erhalten und müssen
nach erneutem Lesen neu bestätigt werden. Während einer Operation den Prüfling
angeschlossen lassen. Ein anderer Prüfling mit exakt gleichem EEPROM-Inhalt lässt
sich anhand des Inhalts allein nicht unterscheiden.
