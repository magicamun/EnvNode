# Hardwareerkennung und Provisioning

Die Seite **Hardware** zeigt, welches Mainboard und welche Module EnvNode erkannt
hat. Das daraus gewählte Boardprofil bestimmt die verfügbaren Anschlüsse.
Prüfe diese Zuordnung vor dem Einrichten von Sensoren und Ausgängen.

**Provisioning** schreibt die Identität einer Baugruppe in deren EEPROM.
Es wird bei leerem oder fehlerhaftem Datensatz benötigt. Ein bereits korrekt
erkanntes Board muss weder bei jedem Start noch nach jedem Firmwareupdate neu
provisioniert werden.

## Welche Daten liegen wo?

| Speicher | Inhalt | Bedeutung für die Bedienung |
| --- | --- | --- |
| Mainboard-EEPROM | Boardidentität und Portzuordnung | Profil, Revision und zulässige Anschlüsse des Mainboards |
| Modul-EEPROM | Modulbeschreibung (Descriptor) und individuelle Fertigungsdaten | Art und Fähigkeiten der Erweiterung im jeweiligen Slot |
| ESP32-Konfigurationsspeicher (NVS) | Netzwerk, Gerätename, Sensoren, Aktoren und weitere Einstellungen | Deine Anwendungskonfiguration |

Ein geänderter Gerätename ist keine neue Hardwareidentität. Eine
[Konfigurationssicherung](maintenance.md#konfiguration-sichern-und-wiederherstellen)
ersetzt keine separate Prüfung der EEPROM-Inhalte.

<span id="selbstbeschreibende-portzuordnung"></span>

## Erkennung zuerst prüfen

| Bereich auf der Hardwareseite | Was du prüfst |
| --- | --- |
| Platinenidentität | Profil und Hardwareversion stimmen mit der realen Platine überein. |
| Quelle und Datensatzstatus | Die Zuordnung stammt aus einem gültigen EEPROM-Datensatz. `BuildFallback` ist lediglich eine Vorgabe des Firmwarebuilds, kein Nachweis korrekt provisionierter Hardware. |
| Normalbetrieb | Die Firmware lässt den Betrieb mit dieser Identität zu; bei einer Sperre zuerst die Ursache klären. |
| Modulidentität | Modul, Revision und tatsächlicher Slot stimmen überein. |
| Erkannte Modulgeräte | Einzelne Geräte und Treiber des Moduls sind verfügbar und ihre Anschlüsse aufgelöst. |

Ein gültiger Descriptor bedeutet nicht automatisch, dass jeder benötigte Treiber
vorhanden ist. `MissingDriver` bezeichnet einen fehlenden Treiber; das lässt sich
nicht durch Auswahl eines nur ähnlich benannten Modulprofils beheben.

Wenn ein EEPROM nicht erreichbar ist, zuerst Versorgung und Anschluss anhand der
Hardwareanleitung prüfen. Ein I²C-Scan unter **Diagnose** kann helfen:
Mainboard-EEPROM `0x50`, Slot A `0x52`, Slot B `0x53`, jeweils auf I2C0, soweit
der Slot am Mainboard vorhanden ist. Eine Antwort bestätigt noch keinen gültigen Inhalt.

<span id="nachladbare-profile-ab-envnode-083-tester-080"></span>

## Passendes Profil wählen

<span id="unterstutzte-mainboard-revisionen"></span>
<span id="weather-revision-02-provisionierung"></span>
<span id="mini-revision-08-provisionierung"></span>

Die tatsächliche PCB-Revision an der Platine feststellen und das dazu passende
Profil im Dialog wählen. Bild und Versionsangabe dienen der Gegenprüfung.
Keine höhere oder ähnlich aussehende Revision als Ersatz für ein fehlendes Profil wählen.
Die [Kompatibilitätsübersicht](../compatibility.md) ordnet PCB-Revisionen und
Firmwareunterstützung zu, einschließlich zusammengefasster Patchrevisionen.

Im Abschnitt **Profilkatalog** lassen sich verfügbare Onlineprofile laden
beziehungsweise aktualisieren. Dafür braucht der Browser sowohl Internetzugang
als auch eine Verbindung zum EnvNode. Eingebaute Profile bleiben ohne Internet
im selben Dialog verfügbar. Die angezeigte Betriebsmindestversion betrifft den
späteren Betrieb; die Möglichkeit, ein Profil zu schreiben, bestätigt noch keine
Treiberunterstützung der laufenden Firmware.

<span id="gemeinsamer-dialog-ab-envnode-085-tester-082"></span>

<span id="eeprom-inhalt-prufen-ab-envnode-086-tester-083"></span>

## Felder des Provisioning-Dialogs

| Feld / Bereich | Bedeutung |
| --- | --- |
| Hardwaretyp | Board oder Modul; legt das Ziel des Schreibens fest. |
| Profil | Passender Typ und Revision der realen Baugruppe. |
| Slot | Nur beim Modul: tatsächlicher EEPROM-Slot A oder B. |
| Seriennummer | Individuelle Kennzeichnung; leer bedeutet noch nicht zugewiesen. Beim Board bis zu 17 Buchstaben, Ziffern, `-`, `_` oder `.`. |
| Produktionscharge und Fertigungsdatum | Optionale, zur Baugruppe gehörende Angaben; Datumsformat wie im Formular angegeben. |
| Aktuell im EEPROM | Frisch gelesener Inhalt einschließlich Lesestatus und vorhandener Fertigungsdaten. |
| EEPROM erneut lesen | Istzustand vor einem Schreibvorgang aktualisieren. |
| Vorhandene Fertigungsdaten übernehmen | Vorhandene Angaben ausdrücklich in die Eingabefelder kopieren. |
| Neu zu schreiben | Geplanter Inhalt und Abweichungen gegenüber dem gelesenen Zustand. |
| Schreibbestätigung | Bestätigung der aktuellen Auswahl und Angaben; nach Änderungen erneut kontrollieren. |

Die Profilvorlage beschreibt die Hardwareart. Seriennummer, Charge und Datum
bezeichnen die individuelle Baugruppe; sie sollten nicht unbesehen von einer
anderen Baugruppe übernommen werden.

<span id="1-mainboard-ohne-modul-provisionieren"></span>

## Mainboard schreiben

1. Unter **Hardware** im Provisioning-Dialog **Board** auswählen.
2. Passendes Profil und Revision wählen.
3. EEPROM-Inhalt lesen und vorhandene Fertigungsdaten prüfen. Bei Bedarf
   ausdrücklich übernehmen oder die eigenen Angaben eintragen.
4. **Neu zu schreiben** mit der realen Baugruppe und dem bisherigen Inhalt vergleichen.
5. Schreibbestätigung setzen und **Board provisionieren** ausführen.
6. Erfolgreiches Schreiben einschließlich Rückleseprüfung abwarten.
7. EnvNode neu starten und Profil, Revision, Datensatzstatus und Normalbetrieb prüfen.

Das Schreiben ändert die aktive Portzuordnung des laufenden Boots nicht.
Deshalb ist der Neustart Teil des Ablaufs. Ein Firmwareupdate schreibt die
EEPROM-Revision nicht automatisch auf einen neueren Hardwarestand um.

## Modul schreiben

1. Modul und tatsächlichen Slot feststellen; Verdrahtungsänderungen nur spannungsfrei vornehmen.
2. Im Dialog **Modul** und das genau passende Profil auswählen.
3. Slot A oder B wählen, EEPROM lesen und individuelle Fertigungsdaten prüfen.
4. Geplanten Inhalt kontrollieren, Schreibbestätigung setzen und **Modul provisionieren** ausführen.
5. Schreib- und Rückleseergebnis abwarten.
6. EnvNode neu starten; anschließend Modulidentität und erkannte Modulgeräte prüfen.

Fehlt das passende Profil, den Vorgang nicht mit einer anderen Modulvorlage
fortsetzen. Die Identity-Adresse des Moduls ist unabhängig von Adressen seiner
Sensoren oder ADCs. Beispielsweise wählen die Busjumper von AnalogHydroPressure
den ADC-Bus, nicht das Identity-EEPROM.

## Bei Lesefehlern oder veränderten Daten

Die Firmware prüft den EEPROM-Inhalt unmittelbar vor dem Schreiben erneut.
Bei Änderungen oder Lesefehlern wird nicht geschrieben. Eingaben kontrollieren,
EEPROM erneut lesen und die geplante Änderung neu bestätigen. Während Lesen und
Schreiben Baugruppe und Versorgung nicht wechseln.

Bei beschädigter Portzuordnung oder widersprüchlicher Identität kann der
Normalbetrieb gesperrt bleiben. Ursache anhand der Diagnose und Protokolle
klären, bevor mit einem passenden Profil neu provisioniert wird.

## Was danach noch nötig ist

Ein erkannter Sensor muss für die Messung weiterhin unter **Sensoren** konfiguriert
werden. Ein kompatibler Modul-Descriptor kann Modulaktoren bereitstellen; Name,
Aktivierung und Kanalzuordnung sind zu prüfen. Controller und Displays werden
anschließend passend zur Anwendung eingerichtet.

[DuoRelay in Betrieb nehmen](../modules/duo-relay/commissioning.md) ·
[Sensoren einrichten](sensors.md) · [Aktoren einrichten](actuators.md)

## Mit dem separaten Tester provisionieren

Die normale EnvNode-Firmware schreibt die eigene Hardware. Die separate
Test-Firmware schreibt dagegen einen extern angeschlossenen Prüfling und
benötigt die dafür vorgesehene Testhardware. Ihr Ablauf steht einmalig unter
[Test/Provisioning: Provisioning und Profile](../test-provisioning/operation/provisioning-profile.md).

Technische Details der Identität und Portzuordnung sind im
[BoardIdentityRecord im Repository](https://github.com/magicamun/EnvNode/blob/main/docs/BoardIdentityRecord.md) dokumentiert.
