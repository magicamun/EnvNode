# Nachladbare Hardwareprofile — EnvNode 0.8.6 / Tester 0.8.3

EnvNode-eigene Inhalte unterliegen der Repository-LICENSE
(LicenseRef-EnvNode-Use-and-Redistribution-1.0).

## Durchgängiger Ablauf

Die normale Firmware bietet die Auswahl in ihrer Hardwareverwaltung an; der
Tester behält sein eigenes Menü **Provisioning** für den externen Prüfling.
Beide verwenden denselben Katalog, Decoder, Modul-Template-Encoder und verifizierte
EEPROM-Schreibpfade. Die Auswahl zeigt Board-/Modulname, Revision, Bild und
Betriebsmindestversion. Die Bilder lädt der Browser direkt von docs.envnode.de.
Seriennummer, Produktionscharge und Fertigungsdatum stehen für Boards und Module
gleichermaßen zur Verfügung; nur die Slotauswahl ist modulspezifisch. Mini, Weather und
DuoRelay verwenden dieselben revisionsbezogenen 3D-Vorderansichten wie die
Hardwaredokumentation. Für Mainboard 0.3 liegt kein Doku-Rendering vor.

Seit EnvNode 0.8.5 / Tester 0.8.2 gibt es einen gemeinsamen Dialog: zuerst
**Board** oder **Modul**, danach genau eine nach Typ gefilterte, alphabetisch sortierte Profilauswahl
(mit natürlicher Sortierung der Revisionsnummern).
Bild, Revision und Betriebsmindestversion stehen neben den Fertigungsfeldern;
auf schmalen Displays untereinander. Onlineprofile ersetzen passende eingebaute
Einträge derselben Art, Bezeichnung und Revision. Quelle und Offline-Rückfall
bleiben erkennbar; zusätzliche Onlineprodukte erscheinen ohne zweite Auswahl.
Die Bestätigung wird bei Profilwechsel oder geänderten Fertigungsangaben gelöscht.
Anschlusshinweise und Katalogaktionen sind aufklappbar. Der Tester adressiert
weiterhin den externen Prüfling, die normale Firmware das eigene Board.

Boardseriennummern behalten die ENID-Grenze von 17 Zeichen, Module bis zu 64;
Charge ist auf 64 und das ISO-Datum auf 10 Bytes begrenzt. Alle Felder sind
optional. Mit Board-Charge oder -Datum wird ENPM-Payload v2 geschrieben und die
effektive Mindestversion auf mindestens EnvNode 0.8.5 angehoben. Die Anzeige
berücksichtigt dies bereits während der Eingabe. Ohne diese Angaben bleibt
Payload v1 erhalten; alte Datensätze werden weiterhin gelesen. Gelesene
Board-Fertigungsdaten erscheinen zusammenhängend im Dialog. Die normale
Firmware zeigt dabei den beim Start gelesenen Stand; Änderungen gelten nach Neustart.

Beim ersten Öffnen der Seite erfolgt ein begrenzter Abruf von
`https://docs.envnode.de/assets/hardware/v1/provisioning-catalog.cbor`.
**Online-Profile aktualisieren** wiederholt den Abruf ausdrücklich. Der Browser benötigt
Internetzugang und eine Verbindung zum Gerät; die Geräteuhr ist hierfür nicht nötig. Das gewählte
Profil wird erst beim bestätigten Schreiben geladen; eine Bild-/Profilauswahl
schreibt kein EEPROM. Ohne Bestätigung beginnt auch kein Profildownload.

Ein Profil kann provisioniert werden, obwohl die Firmware seine Treiber nicht
enthält oder die Betriebsmindestversion nicht erfüllt. Die normale Firmware
entscheidet beim Start über den Betrieb. Belegte Modulports bleiben unabhängig
davon reserviert. Nach einem Schreibvorgang im laufenden EnvNode ist ein Neustart
zur Übernahme der Hardwarekonfiguration erforderlich; der Tester startet für
seinen externen Prüfling nicht neu.

Die neue Form bindet die Auswahl an Profil-ID **und** Inhalts-Hash. Ein veraltetes
Formular darf daher nach einer Katalogänderung nicht unbemerkt etwas anderes
schreiben. Eingebaute Profile bleiben im gemeinsamen Dialog ohne Internet verfügbar.
Ein bereits geladenes Profil wird innerhalb der Sitzung wiederverwendet; ein
fehlgeschlagener Download oder Refresh ersetzt keine gültigen Daten. Es gibt
keinen persistenten Downloadcache. Nach dem Provisioning benötigt die normale
Firmware dank ENPM/ENHD beim Neustart kein Internet zur Portauflösung.

Während einer laufenden Anschlussprüfung sperrt der Tester Katalogabruf,
Profildownload und sämtliche Schreibpfade. Der Browser prüft vor jedem Abruf den lokalen Sperrstatus. Downloads laufen
asynchron mit Zeitlimit; der Schreibendpunkt prüft die Sperre erneut.

## Eine Quelle und zwei EEPROM-Projektionen

Boarddatensätze: `firmware/hardware-profiles/*.json`.
Moduldatensätze: `firmware/hardware-profiles/modules/*.json`.

Der gemeinsame Generator erzeugt:

- Firmwaretabellen, ENPM-Vorlagen und kompatible bisherige Tester-v1-Prüfprofile;
- den binären Provisioning-Katalog und hashgebundene ENPM-/ENHD-Vorlagen;
- die eingebaute DuoRelay-Vorlage aus demselben Moduldatensatz wie der Download.

```sh
python3 firmware/scripts/generate_hardware_profiles.py
python3 firmware/scripts/generate_hardware_profiles.py --check
```

Board-Provisioning ergänzt die individuelle Seriennummer in der ENPM-Vorlage
und schreibt ENPM vor ENID. Modul-Provisioning ersetzt die Null-UUID durch eine
neue UUIDv4 und ergänzt Seriennummer, Charge und Datum. Alle anderen Felder,
auch unbekannte zukünftige Descriptor-Felder, bleiben erhalten. Der generische
Descriptor-Decoder validiert das Ergebnis vor dem Schreiben. Herstellungsdaten
und UUIDs werden nicht im veröffentlichten Katalog gespeichert.

Anfangs enthält der Katalog Mainboard 0.3, Weather 0.2/0.3, Mini 0.8/0.9 und
DuoRelay 0.3. Mainboard 0.3 hat weiterhin kein zugehöriges Prüfprofilbild.
Für DuoRelay wird die vorhandene Assembly-Ansicht verwendet; ein elektrisches
Modulprüfprofil ist noch nicht definiert. SD und SX1262-Remote erhalten erst mit
geprüften Hardwaredaten eigene Produktprofile. Der Decoder benötigt für neue
Profil-IDs und Modul-Treiber keine neue Produktliste in der Firmware.

## Binäres Katalogformat 1

Katalog: `[1, entries]`, höchstens 16 Einträge und 16384 Bytes.
Eintrag (neun Felder):

```text
[id, name, kind, [revisionMajor, revisionMinor],
 [minimumFwMajor, minimumFwMinor, minimumFwPatch],
 imagePath, filename, byteCount, sha256]
```

`kind` ist 0 für eine ENPM-Boardvorlage und 1 für eine vollständige
ENHD-0.1-Modulvorlage. IDs sind höchstens 95 Bytes lang und enthalten ausschließlich
kleine ASCII-Buchstaben, Ziffern, Bindestrich und Punkt; Pfade und `..` sind
verboten. Namen sind höchstens 95 Bytes lang und enthalten keine Steuerzeichen.
Bildpfade beginnen mit `assets/`, bleiben auf dem Dokumentationshost und erlauben
keine URL, Query oder Pfadtraversierung. Der Dateiname muss exakt sein:

```text
id.sha256.portmap.cbor
id.sha256.module.cbor
```

Der Katalog enthält einen vollständigen SHA-256 in kleinen Hexzeichen und die
exakte Dateilänge. Downloads sind auf höchstens 2048 Bytes begrenzt, ENPM auf
1024 Bytes. Zusätzlich müssen Name, Revision und Betriebsmindestversion des
geladenen Datensatzes zum Katalog passen. Board-Seriennummern und alle
Modul-Herstellungsdaten müssen in Vorlagen leer sein; Modul-UUIDs müssen null sein.
Doppelte IDs, unbekannte Formate, nichtkanonische/unbestimmte CBOR-Daten und
nachgestellte Bytes werden abgelehnt. Geänderte Inhalte oder Metadaten einer
noch im Katalog bzw. im geladenen Modell gehaltenen ID werden im selben Lauf
abgelehnt. Eine Vertragsänderung bekommt eine neue Profil-ID/Hardware-Revision.
Diese Konfliktprüfung ist nicht über Neustarts hinweg persistent.

## Transport, Speicher und Veröffentlichung

Seit EnvNode 0.8.4 / Tester 0.8.1 lädt der Browser Katalog und Vorlagen über
HTTPS mit anonymem CORS, ohne Redirects. Er prüft Länge und SHA-256 der Vorlage
und sendet den ausgewählten CBOR-Katalogeintrag und die Vorlage an den lokalen
Schreibendpunkt. Ein kleiner mitgelieferter SHA-256-Client funktioniert auch
auf einer lokalen HTTP-Seite ohne Web Crypto. Bereits geprüfte Vorlagen bleiben
für die Dauer der geöffneten Seite im Browser-RAM verfügbar.

Der Browserclient selbst liegt unter einem hashgebundenen Dateinamen auf
docs.envnode.de und wird asynchron mit Subresource Integrity geladen. Sein
Ausfall blockiert die eingebauten lokalen Formulare nicht. Die MIT-Lizenz der
verwendeten Bibliothek js-sha256 ist im veröffentlichten Client enthalten.

Der ESP begrenzt Uploadgrößen und prüft CBOR-Struktur, Profil-ID, ausgewählten
Hashschlüssel, Metadaten und EEPROM-Inhalt vor dem bestehenden verifizierten
Schreibpfad. Er prüft den SHA-256 nicht erneut und authentifiziert die Herkunft
des Uploads nicht unabhängig. Das Vertrauensmodell umfasst Browser, Docs-Host
und die lokale Verbindung zum Gerät; eine Public-Key-Signaturprüfung ist nicht
implementiert. Bilder lädt der Browser über HTTPS ohne zusätzliche Hashprüfung.
Die normale Firmware benötigt dadurch keinen TLS-Client für Provisioning.
Der getrennte Download von Anschlussprüfprofilen im Tester verwendet weiterhin
dessen bisherigen HTTPS-Transport.

Ein Push nach main veröffentlicht die Dateien über den bestehenden Docs-Workflow.
Vor dessen erfolgreichem Abschluss kann der neue Endpunkt noch HTTP 404 liefern;
die eingebauten Profile funktionieren dabei weiter. Bereits veröffentlichte
hashgebundene Dateien werden nicht überschrieben oder gelöscht.

Siehe auch [ENPM/ENID](BoardIdentityRecord.md),
[gemeinsame Hardwareprofile](UnifiedHardwareProfiles.md) und
[bisherige Remote-Prüfprofile](TestBoardRemoteProfiles.md).

## EEPROM-Istzustand und Schreibschutz ab 0.8.6 / Tester 0.8.3

Der gemeinsame Dialog liest das ausgewählte Ziel beim Öffnen sowie beim Wechsel
zwischen Board und Modul bzw. Slot A/B frisch aus dem EEPROM. **Aktuell im EEPROM**
zeigt Lesestatus, Name/Revision, Seriennummer, Charge und Datum. Status unterscheiden
sich in gültig, leer, beschädigt/nicht interpretierbar und nicht erreichbar.
**EEPROM erneut lesen** aktualisiert ausschließlich die Anzeige, ohne Schreibzugriff
oder Änderung der aktiven Laufzeitkonfiguration. Es gibt keinen Internetabruf dafür.

**Neu zu schreiben** bleibt unabhängig: keine automatische Übernahme gelesener
Fertigungsangaben. **Vorhandene Fertigungsdaten übernehmen** kopiert nur auf Wunsch
Seriennummer, Charge und Datum eines gültigen Datensatzes. Abweichungen zwischen
Anzeige und Eingaben stehen ausdrücklich in der Schreibzusammenfassung.
Lesen, Übernahme, Änderungen an Angaben sowie Ziel-/Profilwechsel löschen die Bestätigung.

Der Leser bildet eine begrenzte Momentaufnahme der gesamten 4096 Bytes und einen
64-Bit-FNV-1a-Fingerabdruck mit Zielkennung (Board, Slot A oder Slot B). Derselbe
Puffer liefert auch die dekodierte Anzeige; es werden keine Start-Caches verwendet.
Vor dem Schreiben im gemeinsamen Dialog wird das ausgewählte EEPROM erneut gelesen.
Fehlender/abweichender Fingerabdruck oder Lesefehler ergibt HTTP 409 ohne Schreibzugriff.
Die Oberfläche liest dann erneut, behält die eingegebenen neuen Werte und verlangt
eine neue Prüfung/Bestätigung. Bei nicht erreichbarem EEPROM bleibt Schreiben gesperrt;
leere und beschädigte Datensätze dürfen bewusst neu provisioniert werden.

Der Fingerabdruck dient der Erkennung geänderter Inhalte, nicht der Authentifizierung.
Physisch verschiedene EEPROMs mit exakt gleichem Inhalt lassen sich damit nicht
unterscheiden. Ein Wechsel während einer einzelnen Lese-/Schreiboperation ist
nicht atomar ausschließbar; während einer Operation bleibt der Prüfling angeschlossen.
Bestehende POST-Integrationen ohne `snapshotToken` behalten ihren bisherigen
Bestätigungsvertrag. Der gemeinsame Dialog sendet das Feld immer; ein leerer Wert
wird abgelehnt. Die Anschlussprüf-Sperre gilt auch für den neuen Leseendpunkt.
