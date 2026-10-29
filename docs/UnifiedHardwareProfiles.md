# Gemeinsame Hardwareprofile

EnvNode-eigene Inhalte unterliegen der Repository-LICENSE
(LicenseRef-EnvNode-Use-and-Redistribution-1.0).

## Verbindliche Trennung

Ein Profil beschreibt die physische Hardware. Provisioning benötigt das unterstützte
EEPROM-Format, jedoch keine Betriebstreiber. `minimumRuntimeFirmwareVersion` bzw.
`compatibility.minimumFirmwareVersion` im ENHD-Deskriptor gilt ausschließlich für
Betrieb. Auch eine Firmware, die ein Modul niemals betreiben wird, darf dessen
validen Datensatz schreiben. Fehlende Treiber bleiben unabhängig von einer
Versionsnummer ein Grund, das Modul nicht zu aktivieren.

Nach dem Schreiben werden Speicherintegrität und Wiedererkennung geprüft.
`Success` bedeutet erfolgreich provisioniert; `compatibilityStatus` beschreibt
separat die Nutzbarkeit. Eine höhere Mindestversion oder ein fehlender Treiber
führt nicht zu einem Schreibfehler.

## Eine gepflegte Quelle

`firmware/hardware-profiles/*.json` enthält die fünf vorhandenen Boardrevisionen:
Mainboard 0.3, Weather 0.2/0.3 und Mini 0.8/0.9. Jeder Datensatz verbindet:

- Identität und Revision;
- Provisioning-Bus/-Adresse und Capability-/GPIO-Zuordnung einschließlich Slots;
- SPI- und I²C-Zuordnung;
- Mindestversion für den Normalbetrieb;
- Bild-URL und vollständige Testdefinition, soweit vorhanden.

Mainboard 0.3 hat bisher keine Anschlussprüfdefinition und kein zugehöriges
Prüfprofilbild; beide Felder bleiben ausdrücklich `null`. Die initiale
Betriebsmindestversion der migrierten Boarddatensätze ist konservativ 0.8.2,
der dokumentierte gemeinsame Unterstützungsstand. Dies ist keine nachträgliche
Behauptung über die allererste historisch lauffähige Firmwareversion.

Die SVG-Quellen liegen unter `firmware/hardware-profiles/images/`.
`testGeneration` hält die Herkunft der PCB-Auswertung fest. Die vorhandene
PCB-Extraktionsfunktion bleibt als Werkzeug erhalten; reguläres Generieren liest
die gemeinsamen Datensätze und benötigt weder KiCad noch fremde Arbeitskopien.
Bei Hardwareänderungen sind GPIO-Zuordnung und Anschlussprüfdaten gemeinsam zu
prüfen; die Zusammenführung ersetzt keine elektrische Validierung.

```sh
python3 firmware/scripts/generate_hardware_profiles.py
python3 firmware/scripts/generate_hardware_profiles.py --check
```

Dieser eine Aufruf erzeugt Firmwaretabellen, Identitätskatalog,
JSON-Downloads unter `docs/site/assets/hardware/v1/` und die bisherigen
Tester-CBOR-/C++-/Bilddateien. Generierte Tabellen und Dateien nicht separat
pflegen. Die veröffentlichten Tester-v1-Dateien bleiben bytegleich.
Dateinamen der neuen JSON-Datensätze enthalten ihren SHA-256; der Katalog referenziert
sie. Diese Dateien werden erst mit einem regulären Docs-Deployment öffentlich.

## Physische Belegung und Anzeige

Erkannte, valide Moduldeskriptoren reservieren die genannten AUX-/CS-Ports auch
bei `FirmwareTooOld`, `MissingDriver` oder anderer fehlender Betriebsunterstützung.
SPI-Anforderungen reservieren zusätzlich die drei Buspins aus dem Boardprofil.
I²C-Anforderungen mit `properties.address` reservieren die jeweilige Busadresse;
weitere Adressen auf demselben I²C-Bus bleiben verfügbar. Die EEPROM-Adresse eines
erkannten Moduls ist ebenfalls reserviert.

Die GPIO-Auswahl in Sensor- und Aktorformularen filtert diese Belegung. Die
Konfigurationsvalidierung schützt zusätzlich direkte Schreibaufrufe und
Backup-Restore. Beim Laden werden kollidierende manuelle Sensoren/Aktoren im
RAM deaktiviert; die gespeicherte Konfiguration bleibt für den Fall erhalten,
dass das Modul später entfernt wird. Deskriptorbasierte Aktoren werden weiterhin
nur bei vorhandener Betriebsunterstützung erzeugt.

Die Hardwareübersicht zeigt erkannte Module, deren Betriebsstatus und die
benötigte Firmwareversion. Geräte eines nicht unterstützten Moduls bleiben im
Inventar sichtbar. Beide Provisioning-Formulare beziehen Boarddaten und
Mindestversion aus demselben Katalog. Die normale Firmware zeigt das Profilbild
per HTTPS, der Tester behält zusätzlich seine bestehende Offline-Bildansicht.

## EEPROM und noch ausstehende Integration

Diese Änderung erhält ENID und ENHD 0.1. Der ENID-Boarddatensatz enthält weiterhin
Profil-ID, Revision und Seriennummer; seine GPIO-Tabelle wird aus den gemeinsamen
Quelldaten in die Firmware generiert. ENHD-Moduldatensätze enthalten bereits die
logischen Anforderungen. Bild und Prüfdaten werden nicht ins EEPROM geschrieben.

Die vollständige Umsetzung nachladbarer Provisioning-Profile ist damit noch
nicht abgeschlossen: Die normale Firmware importiert den neuen JSON-Katalog
noch nicht zur Laufzeit. Der Tester lädt weiterhin den kompatiblen v1-Prüfkatalog;
sein externes Provisioning unterstützt ENID-Boards und ENHD-Module an I²C0
unter 0x50 bzw. 0x52/0x53.
Beide Anwendungen verwenden für ENHD dieselbe Validierung und denselben verifizierten
EEPROM-Schreibpfad (`ModuleDescriptorWriter`). Der Tester benötigt dafür keine
Betriebstreiber und prüft die Mindestversion nicht als Schreibvoraussetzung. Er
aktiviert den Provisioning-Bus nur für die gewählte Operation und gibt SDA/SCL
danach wieder frei. Ein laufender Anschlusstest sperrt Board- und Moduloperationen,
einschließlich der alten `/device/...`-Routen. Die UUID wird im Tester mit der
ESP32-Zufallsquelle erzeugt; das Ziel-EEPROM gehört weiterhin zum externen Prüfling.

Die Modulvorlage im gemeinsamen Webformular ist weiterhin DuoRelay 0.3.
SD und SX1262-Remote sind keine erfundenen Produktprofile in diesem Katalog.

Als nächster Integrationsschritt werden ein gemeinsamer begrenzter Laufzeitdecoder,
Profilrepository und generische EEPROM-Projektion benötigt. Neue Boardprofile
benötigen außerdem eine stabile Identitäts-/GPIO-Repräsentation im EEPROM oder
einen persistenten lokalen Profilbestand; ein neuer Download allein macht eine
alte Firmware mit ausschließlich eingebauten ENID-Tabellen nicht kompatibel.
Dieser Schritt muss die bestehenden EEPROM-Daten migrieren können und dieselben
Validierungs-, Hash-, Offline- und Transaktionsregeln wie der Tester einhalten.
