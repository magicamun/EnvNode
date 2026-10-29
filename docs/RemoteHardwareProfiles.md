# Nachladbare Hardwareprofile — EnvNode 0.8.3 / Tester 0.8.0

EnvNode-eigene Inhalte unterliegen der Repository-LICENSE
(LicenseRef-EnvNode-Use-and-Redistribution-1.0).

## Durchgängiger Ablauf

Die normale Firmware bietet die Auswahl in ihrer Hardwareverwaltung an; der
Tester behält sein eigenes Menü **Provisioning** für den externen Prüfling.
Beide verwenden denselben Katalog, Decoder, Modul-Template-Encoder und verifizierte
EEPROM-Schreibpfade. Die Auswahl zeigt Board-/Modulname, Revision, Bild und
Betriebsmindestversion. Die Bilder lädt der Browser direkt von docs.envnode.de.
Die Slot-, Chargen- und Datumsfelder erscheinen für Module.

Beim ersten Öffnen der Seite erfolgt ein begrenzter Abruf von
`https://docs.envnode.de/assets/hardware/v1/provisioning-catalog.cbor`.
**Profile aktualisieren** wiederholt den Abruf ausdrücklich. WLAN und eine
synchronisierte Uhrzeit sind Voraussetzung für die TLS-Prüfung. Das gewählte
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
schreiben. Die bisherigen eingebauten Formulare bleiben ohne Internet verfügbar.
Ein bereits geladenes Profil wird innerhalb der Sitzung wiederverwendet; ein
fehlgeschlagener Download oder Refresh ersetzt keine gültigen Daten. Es gibt
keinen persistenten Downloadcache. Nach dem Provisioning benötigt die normale
Firmware dank ENPM/ENHD beim Neustart kein Internet zur Portauflösung.

Während einer laufenden Anschlussprüfung sperrt der Tester Katalogabruf,
Profildownload und sämtliche Schreibpfade. HTTP-Abrufe sind synchron und zeitlich
begrenzt; währenddessen kann die Oberfläche kurz verzögert reagieren. Es wird
kein Hintergrundthread und kein Netzwerkabruf pro Messpunkt angelegt.

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

Der ESP prüft den festen HTTPS-Host gegen ISRG Root X1 und die Zertifikatszeit.
Redirects sind ausgeschaltet; jede Vorlage wird gegen den Katalog-Hash geprüft.
Die TLS-Verbindung und der Downloadpuffer werden vor dem EEPROM-Schreiben
freigegeben. Katalog und geladenes Modell liegen begrenzt im Heap; es bleiben
höchstens ein geladener Datensatz und ein vorbereiteter Ersatz erhalten.
Der große Modul-Decoder wird nur während der jeweiligen Operation angelegt.

HTTPS und Hashbindung vertrauen dem Dokumentationshost. Eine davon unabhängige
Public-Key-Signaturprüfung ist weiterhin nicht implementiert. Der Browser nutzt
für Bilder HTTPS; er prüft deren Inhaltshash nicht zusätzlich.

Ein Push nach main veröffentlicht die Dateien über den bestehenden Docs-Workflow.
Vor dessen erfolgreichem Abschluss kann der neue Endpunkt noch HTTP 404 liefern;
die eingebauten Profile funktionieren dabei weiter. Bereits veröffentlichte
hashgebundene Dateien werden nicht überschrieben oder gelöscht.

Siehe auch [ENPM/ENID](BoardIdentityRecord.md),
[gemeinsame Hardwareprofile](UnifiedHardwareProfiles.md) und
[bisherige Remote-Prüfprofile](TestBoardRemoteProfiles.md).
