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
Modul-Provisioning ist in dieser Test-Firmware deaktiviert.

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
