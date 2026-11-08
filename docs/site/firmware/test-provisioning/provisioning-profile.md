# Provisioning und Prüfprofile

## Zwei getrennte Arbeitsabläufe

Die passive Anschlussprüfung arbeitet am spannungslosen Prüfling über Multiplexer.
Provisioning nutzt dagegen einen **dedizierten I²C-Bus** und schreibt eine
Boardidentität in ein externes EEPROM. Das Umschalten der Multiplexer ersetzt
keine I²C-Verbindung.

| Bus / Anschluss | Nutzung |
| --- | --- |
| J2-Testpigtail | Einzelne Kontakte als LOW/HIGH-Messsignal auswählen |
| I2C0-Provisioning-Anschluss | Echter I²C-Master: SDA GPIO16, SCL GPIO5, EEPROM 0x50 |

Nicht beide Arbeitsabläufe gleichzeitig verkabeln. Vor einem Wechsel Versorgung
trennen und die Verkabelung gezielt ändern. Eine Anzeige oder Profilauswahl allein
schreibt noch nichts ins EEPROM.

Die Boardauswahl ist ab 0.12.1 nach Namen und danach numerisch aufsteigender
Revision sortiert. Die gelesene Identität bleibt ausgewählt. Für Weather 0.3 sind
Lesen, Schreiben, Rücklesen und Erhalt nach vollständigem Stromausfall praktisch
bestätigt; die übrigen Boardtypen müssen bei Bedarf separat real erprobt werden.

## Externe Boardidentität schreiben

1. Alle Messpigtails, das Referenzkabel und den Input-Adapter vom Prüfling trennen.
   Prüflings-ESP32 entfernt lassen.
2. Dediziertes Provisioning-Kabel mit SDA, SCL, GND und der vorgesehenen
   3,3-V-Versorgung anschließen. Pinbelegung des konkreten Zielboards prüfen;
   keine zweite Versorgung parallel anschließen. Der Controller schaltet diese
   Versorgung nicht softwareseitig ein oder aus.
3. **Provisioning** (`/hardware`) öffnen und den Zustand des externen EEPROMs lesen.
4. Boardtyp und **genaue Revision** wählen. Bei leerem EEPROM ist die Auswahl
   absichtlich nicht vorbelegt. Seriennummer nach dem angezeigten Formular setzen.
5. Schreibbestätigung setzen und den Schreibvorgang starten. Die Firmware schreibt
   und liest zur Verifikation zurück. Den angezeigten Erfolg beziehungsweise Fehler prüfen.
6. Vor der passiven Boardprüfung Provisioning-Kabel wieder vollständig entfernen.

Herstellungsprofile sind Mainboard 0.3, Weather 0.2/0.3 und Mini 0.8/0.9. Die
Hardwareidentität und ihre Revision sind kein Prüfergebnis. Ein provisioniertes
Board ist dadurch noch nicht elektrisch geprüft. Die Bestätigung bezieht sich auf
das externe Ziel; der Test-Controller muss anschließend nicht neu starten.
Seit 0.13.0 nutzt der Tester denselben aktuellen Provisioning-Dialog und Core wie
EnvNode. EEPROM-Anzeige, explizite Übernahme, Vorher/Nachher-Vergleich, Profile und
Schreib-/Rückleseprüfung folgen dem [gemeinsamen Ablauf](../provisioning.md).
Die Identitäts-/Portprofile sind von den Durchgangs-Prüfmatrizen getrennt.

## Modul über das Mainboard provisionieren

1. Alle Messverbindungen und den Input-Adapter entfernen; Mainboard-ESP32 und
   eigene Versorgungen entfernt lassen.
2. Stromlos das Modul in den passenden Slot des Mainboards stecken und den
   dedizierten Provisioning-Bus an I2C0 des Mainboards anschließen.
3. Im gemeinsamen Hardwaredialog Modul und Slot auswählen: Slot A ist EEPROM
   0x52, Slot B 0x53. Das Mainboard-EEPROM an 0x50 wird dabei nicht überschrieben.
4. Zuerst das vorhandene EEPROM lesen und vorbefüllte beziehungsweise übernommene
   Werte bewusst prüfen. Profil und Fertigungsdaten wählen, Schreibvorschau prüfen
   und den Schreibvorgang ausdrücklich bestätigen.
5. Schreiben und Rücklesen kontrollieren; anschließend stromlos machen und die
   Identität erneut lesen. Der Tester muss keinen Modul-Betriebstreiber laden.

Der Modul-Testadapter und die Mess-Multiplexer sind für diesen Ablauf nicht nötig.
Die Gesamtbestückung darf nur mit der vorgesehenen 3,3-V-Versorgung betrieben
werden; keine Aktoren, Lasten oder zusätzliche Versorgung anschließen. Bei Bedarf
muss die Betriebselektronik eines Moduls vom EEPROM-Provisioning getrennt versorgt
werden. Die Firmware kann die Versorgung nicht abschalten.

Board-Provisioning auf Weather 0.3 wurde im bisherigen Testeraufbau praktisch
bestätigt. Der neue gemeinsame Core und Modul-Provisioning sind automatisiert
geprüft, benötigen nach dem Merge aber noch den realen Schreib-/Kaltstarttest.
Neue Controller-/Adapterrevisionen bleiben ebenfalls als noch nicht praktisch
abgenommen gekennzeichnet.

## Drei Zuordnungen auseinanderhalten

| Daten | Aufgabe |
| --- | --- |
| Fixture-/Adaptertabelle | ESP32-Steuer-GPIOs und Multiplexeradressen zu physischen Testkontakten; getrennte Controller-/Adapterprofile, siehe Hardwareauswahl |
| Mainboard-Prüfprofil | Erwartete Kontaktverbindungen, Anschlusspositionen und Boardbild für die Anschlussprüfung |
| Port-/GPIO-Profil der normalen EnvNode-Firmware | Bedeutung der GPIOs, Ports, I²C-Busse und Modulslots im normalen Gerätebetrieb |

Ein Remote-Prüfprofil ändert weder die Verdrahtung des Adapters noch die
Hardwareansteuerung der normalen EnvNode-Anwendung. Ein neuer Adapter braucht ein
eigenes Fixture-Mapping. Die [Hardwareauswahl](hardwareprofile.md) unterstützt
bereits Legacy und ESP32 DevKitC; S3 folgt später.

## Remote-Prüfprofile

Die Test-Firmware lädt den Katalog von
[`assets/testboard/v1/catalog.cbor`](../../assets/testboard/v1/catalog.cbor).
Aus KiCad erzeugte Profile und Bilder liegen versioniert im Repository unter
`docs/site/assets/testboard/v1/`. Commit und Push nach `main` veröffentlichen sie
über den regulären Docs-Workflow auf docs.envnode.de. Ein Feature-Branch
veröffentlicht diese Dateien nicht automatisch.

Die Firmware prüft HTTPS-Zertifikat, Format, Größenlimits, Profilidentität und
SHA-256. Gleiche Revisions-IDs dürfen nicht mit anderen Inhalten ersetzt werden;
neue Hardwarestände erhalten neue IDs. Eine zusätzliche unabhängige Signatur mit
einem im Gerät hinterlegten öffentlichen Schlüssel ist noch nicht umgesetzt.

Der Browser lädt das zum Profil gehörende Bild direkt über dessen URL. Die
Firmware verarbeitet die kompakten Prüfprofile, ohne das Bild im ESP32-RAM zu
halten. Passende eingebaute Profile bleiben bei fehlendem Netz oder einem
abgelehnten Download verfügbar. Ein reines Remote-Profil ohne eingebauten Ersatz
kann bei Downloadfehler nicht geladen werden. Der Cache liegt im RAM, nicht
persistent im Flash.

Weitere technische Details stehen im Repository in
`docs/TestBoardRemoteProfiles.md` und `docs/TestBoardFirmware.md`.
