# Board-Revisionen: Identität, Laufzeit und Provisioning

EnvNode-eigene Inhalte unterliegen der Repository-LICENSE
(LicenseRef-EnvNode-Use-and-Redistribution-1.0).

Seit EnvNode 0.8.2 / Testboard 0.7.0 gilt ein gemeinsamer Identitätskatalog:

| BoardProfileId | Board | Unterstützte Hardware-Revisionen |
| --- | --- | --- |
| 0 | EnvNode Mainboard | 0.3 |
| 1 | EnvNode Weather | 0.2, 0.3 |
| 2 | EnvNode Mini | 0.8, 0.9 |

Die 32-Byte-ENID-Daten, Seriennummern, CRC und Schreib-/Rückleseverfahren ändern
sich nicht. Vorhandene EEPROMs bleiben lesbar; es gibt keine automatische
Umschreibung einer Hardware-Revision und keine Konfigurationsmigration.

`BoardIdentityProfiles` in EnvNodeIdentity stellt die Fertigungsmetadaten bereit.
Codec/Validierung sowie beide Provisioning-Anwendungen verwenden diese Tabelle.
Die normalen GPIO-/Bus-/Modulressourcen bleiben im EnvNode-Laufzeitkatalog.
Für die neuen Revisionen sind alle 38 ESP32-Netze identisch zum Vorgänger;
Referenzschutz, Abblockung und Jumperausführung ändern deren GPIO-Zuordnung nicht.
Deshalb werden Ressourcenlisten geteilt, aber eigene Revisionsobjekte ausgewählt.

Der Start wählt `selectCurrentBoardProfile(id, revision)` mit der tatsächlich
gelesenen EEPROM-Revision. `currentBoardProfile()` zeigt damit auch 0.9 bzw. 0.3
korrekt an. Eine unbekannte Revision bleibt gesperrt. Die Auswahl bleibt für den
laufenden Start eingefroren. Ein erfolgreiches Selbst-Provisioning verlangt wie
bisher einen ausdrücklichen Neustart; der externe Tester benötigt keinen Neustart.

## Bestehende APIs und Sensor-Branches

Die bestehenden Aufrufe `boardProfile(id)`, `boardProfileAt(index)`,
`boardProfileCount()` und `selectCurrentBoardProfile(id)` bleiben erhalten.
Sie verwenden ihre bisherigen Standardrevisionen (Mainboard 0.3, Weather 0.2,
Mini 0.8). Neue Aufrufer verwenden die explizite Revisionsüberladung.

`IProvisioningAdministration` erhält kompatible virtuelle Standardmethoden für die
Revisionssuche und für das Vorhandensein einer gelesenen Identität. Bestehende
Implementierungen müssen diese nicht ergänzen, wenn sie nur eine Revision anbieten.
Neue Implementierungen können mehrere Revisionen derselben BoardProfileId anbieten.

Die Formulare senden `boardProfileKey`, zum Beispiel `2:0.9` oder `1:0.3`.
Die ID bleibt weiterhin das stabile numerische BoardProfileId im EEPROM.
Ungültige, unbekannte und widersprüchliche Schlüssel werden vor I²C-Zugriff abgelehnt.
Alte POST-Aufrufe mit ausschließlich `boardProfileId=2` behalten Mini 0.8;
sie werden nicht auf 0.9 umgedeutet. Beide Parameter gleichzeitig sind ungültig.
Ein unbekanntes oder leeres externes EEPROM erhält keine voreingestellte Revision:
der Bediener muss Board und tatsächliche Revision auswählen.

Die Tester-Seite zeigt dazu die passende Boardansicht. Sie verwendet dieselben
Prüfdefinitionen und Bildrouten wie die Anschlussprüfung und bevorzugt die darin
angegebene Remote-Bildadresse. Bei Bildfehlern folgt die lokale Ansicht derselben
Revision. Eine Bildauswahl löst keinen EEPROM-Schreibvorgang aus. Modulansichten
folgen separat. Modul-Provisioning im Tester verwendet jetzt den gemeinsamen
ENHD-Schreibpfad an 0x52/0x53, unabhängig von der Betriebsunterstützung.

## Abnahme

45 Tester-Tests und 40 ausgewählte EnvNode-Regressionstests prüfen alte und neue
Revisionen, Codecs, Store/Rücklesen, Laufzeitauflösung, alte API-Aufrufe,
Revisionsformulare, Bestätigung und Fehlerfälle. Beide ESP32-Builds werden geprüft.
Der spätere Hardwaretest umfasst die tatsächliche Busverbindung und EEPROMs,
Multiplexer/ADC und die geschützten Referenzpfade. GPIO16/5 bleiben unverändert.
