# Remote-Prüfprofile (Testboard 0.6.0)

EnvNode-eigene Inhalte unterliegen der Repository-LICENSE
(LicenseRef-EnvNode-Use-and-Redistribution-1.0).

## Betriebsweise

Der Tester fragt nach erfolgreicher WLAN-Verbindung und Zeitsynchronisierung einmal
`https://docs.envnode.de/assets/testboard/v1/catalog.cbor` ab. Die Uhrzeit wird für
die Zertifikatsprüfung benötigt. Der Button **Remote-Profile aktualisieren** erlaubt
eine erneute Anfrage. Eine Aktualisierung oder ein Profilwechsel während einer
laufenden Anschlussprüfung ist gesperrt. Die Abfrage erfolgt mit begrenzten
Verbindungs-, TLS- und Lesetimeouts; sie ist synchron und kann die Webbedienung
kurz verzögern. Es gibt keinen Hintergrund-Thread und keine Anfrage je Messpunkt.

Remote-Einträge stehen zuerst in der Auswahl. Eingebaute Profile ergänzen fehlende
Revisionen. Der konkrete Profildownload erfolgt erst bei Bedarf. Kann ein Profil
nicht geladen oder validiert werden, wird ausschließlich ein eingebautes Profil
mit genau derselben ID verwendet. Ohne passende Definition wird der Ladevorgang
abgelehnt und die bestehende Sitzung erhalten. Ein bereits geladenes Remote-Profil
bleibt für seine Sitzung im RAM verfügbar, auch nach Netzverlust. Es gibt derzeit
keinen persistenten Flash-Cache. Ein Neustart ohne Netz bietet die eingebauten
Profile an. Abgeschlossene Prüfergebnisse werden weiterhin nicht dauerhaft gespeichert.

Die Webansicht erhält ihre Prüfdaten vom Tester. Bei Remote-Profilen enthält die
Antwort zusätzlich `imageUrl` mit einem festen HTTPS-Pfad auf docs.envnode.de.
Das SVG wird direkt vom Browser geladen und belegt keinen Bildpuffer auf dem ESP.
Bei einem Bildfehler versucht der Browser die eingebaute Ansicht derselben ID;
für rein externe Profile kann dann kein Bild angezeigt werden. Die Matrix und
Prüfdaten bleiben unabhängig davon verfügbar. Keine Bauteilwerte werden gerendert.

Diese Funktion setzt keine Mess-GPIOs und startet kein Provisioning-I²C. GPIO16/5
bleiben ausschließlich dem separaten Provisioning zugeordnet. Auf einem Mini lässt
sich der Prüfablauf weiterhin simulieren. Die analoge Prüfung einschließlich
Diodenspannungsabfall wird erst mit der fertigen Testhardware validiert.

## Gemeinsame Quelle mit Provisioning

Die gepflegten Boarddatensätze liegen jetzt unter `firmware/hardware-profiles/`.
`python3 firmware/scripts/generate_hardware_profiles.py` erzeugt daraus sowohl
die Firmware-/Provisioningtabellen als auch die bestehenden Prüfarbeitsdaten.
Details und der Stand der noch ausstehenden Laufzeitintegration stehen in
[UnifiedHardwareProfiles.md](UnifiedHardwareProfiles.md).

## Dateien und Veröffentlichung

`firmware/scripts/generate_testboard_profile.py` erzeugt in einem Durchlauf:

- lesbare JSON-Definitionen und eingebaute C++-/SVG-Daten;
- `docs/site/assets/testboard/v1/catalog.cbor`;
- kompakte Profile mit ID und vollständiger SHA-256 im Dateinamen;
- SVG-Dateien mit vollständiger SHA-256 im Dateinamen und `LICENSE.txt`.

Die neuen Hardwarequellen sind an Commit `98704fb` gebunden. Damit ist kein
Zugriff auf einen veränderlichen Nachbar-Checkout nötig. Die älteren JSON- und
C++-Definitionen für Mini 0.8 / PCB 0.8.1 sowie Weather 0.2 / PCB 0.2.1 bleiben
unverändert. Mini 0.9 / PCB 0.9.0 und Weather 0.3 / PCB 0.3.0 ergänzen den Katalog.

```sh
python3 firmware/scripts/generate_testboard_profile.py
python3 firmware/scripts/generate_testboard_profile.py --check
```

Die Dateien liegen im vorhandenen MkDocs-Quellverzeichnis und werden vom normalen
Dokumentations-Build kopiert. Feature-Branches veröffentlichen keine Änderungen; ein Push nach main nutzt den regulären Docs-Workflow.
**Erst nach Übernahme nach main und erfolgreichem Docs-Deployment ist der neue
öffentliche Pfad verfügbar.** Bis dahin ist HTTP 404 mit eingebautem Rückfall
zu erwarten. Bei Veröffentlichungen müssen alle im Katalog referenzierten Dateien
mitkopiert werden; bereits veröffentlichte Dateien bleiben unverändert.

## Kompaktes Format v1

Alle Arrays sind längenbestimmt; unbestimmte Arrays und nachgestellte Bytes werden
abgelehnt. Ganzzahlen verwenden die kanonische CBOR-Darstellung, Koordinaten
CBOR-Float64. Zeichenketten haben feste Längengrenzen und enthalten keine
Steuerzeichen. IDs und Dateinamen enthalten nur kleine ASCII-Buchstaben, Ziffern,
Punkt und Bindestrich; Pfade, URLs und `..` sind verboten.

Katalog: `[1, entries]`, maximal 8 Remote-Einträge und 8192 Bytes.
Ein Eintrag ist `[id, name, identityRevision, pcbRevision, filename, bytes, sha256]`.
`filename` muss exakt `id + '.' + sha256 + '.cbor'` sein. Doppelte IDs und geänderte
Metadaten/Inhalte derselben ID im bisherigen Katalog oder in noch gehaltenen Modellen werden abgelehnt. Eine inhaltliche
Änderung bekommt eine neue Profil-/PCB-ID. Diese Konfliktprüfung gilt innerhalb
eines Firmwarelaufs; sie ist kein persistenter Schutz gegen alte Kataloge.

Profil: `[1, id, name, identityRevision, pcbRevision, pcbSha256, source, bounds,
inputs, connectors, imageFilename, referencePaths, sourceCommit]`.

- `bounds`: `[x, y, width, height]` in mm, endlich, positive Abmessungen bis 1000 mm.
- `inputs`: 38 oder 40 Einträge `[number, net, x, y]`, fortlaufend ab 1.
- `connectors`: maximal 32 Einträge `[id, label, kind, contacts]` ohne doppelte IDs.
- `kind`: 0 GPIO, 1 I²C, 2 Modul. GPIO/I²C haben 4, Module 14 Kontakte.
- `contacts`: `[number, net, x, y, expectedMask]`, insgesamt höchstens 160 Kontakte.
  Jede Maske ist eine UInt64 und darf nur vorhandene Eingänge referenzieren.
- Alle Kontaktpositionen müssen innerhalb der Bildgrenzen liegen.
- `imageFilename`: 64 kleine Hexzeichen plus `.svg`, stets auf demselben Host.
- `referencePaths`: für 38 Eingänge leer; für 40 Eingänge genau zwei Einträge
  `[input, connector, pin, diode, anodeNet, cathodeNet]`. Eingang 39 ist TP1.2
  für MB_3V3_SYS, Eingang 40 TP1.1 für MB_GND. Die Kathodennetze müssen mit den
  Eingangsnetzen übereinstimmen. Die Prüfrichtung ist Ausgang → Eingang.

Ein Profil darf höchstens 20000 Bytes belegen. Der Decoder verwendet ein begrenztes
Modell mit einem 8192-Byte-Textbereich statt eines JSON-DOM. Downloadpuffer und
TLS-Verbindung werden nach dem Einlesen freigegeben. Es bleiben höchstens ein
aktives und ein vorbereitetes Modell im RAM. Die JSON-Antwort wird nur für den
Browser aufgebaut; eine vorab geprüfte Speicherreservierung begrenzt sie auf
höchstens 64 KiB. Fehlende Speicherreserven führen zum Rückfall oder zu HTTP 503.

## Vertrauensmodell und Grenzen

Der ESP prüft HTTPS gegen ISRG Root X1, den Hostnamen und die Zertifikatsgültigkeit.
Es wird kein `setInsecure` verwendet; Redirects sind deaktiviert. Downloads bleiben
auf dem festen Host und im festen Verzeichnis. Der Profildownload muss mit der
SHA-256 aus dem validierten HTTPS-Katalog übereinstimmen und dessen Metadaten
exakt erfüllen. Danach folgen Format-, Größen-, Bereichs- und Konsistenzprüfungen.

**Eine eigene Public-Key-Signaturprüfung ist noch nicht implementiert.** HTTPS
vertraut dem Dokumentationshost und dessen Veröffentlichung. SHA-256 bindet den
Inhalt an den Katalog, ersetzt aber keine vom Server unabhängige Signatur. Der
Dateiname des Bildes ist in den Profildaten enthalten; dessen Hash wird im Browser
nicht separat geprüft. Der Browser vertraut für das Bild HTTPS und dem Host.

Für einen späteren unabhängigen Signaturnachweis kann der gesamte Katalog mit
seinen Profil-Hashes signiert werden. Private Schlüssel bleiben außerhalb des
Repositorys und der Firmware; die Firmware erhält den dauerhaften Public Key.
Schema-Prüfung, Größenlimits und Offline-Profile bleiben dabei erforderlich.

## Referenzen und Provisioning

Mini 0.9 und Weather 0.3 enthalten 40 Eingänge. Die zwei Referenzpfade werden nur
nach expliziter Prüfung der TP1-/D1-/D2-Netze aus dem Hardwarestand hinzugefügt:
Rail am Anodenpad 2, Referenzabgriff am Kathodenpad 1. Net-Ties werden wie bisher
aufgelöst. Beliebige Dioden, Pull-ups und andere Bauteile werden nicht automatisch
als direkte Verbindungen interpretiert. Im Simulator bestehen Mini mit 2080 und
Weather mit 3440 Vergleichen. Das ist keine elektrische Freigabe der Schwellen.

Die Definitionen bleiben **Prüfprofile**; ENID-Identitätsrevisionen sind davon
getrennt. Seit Testboard 0.7.0 / EnvNode 0.8.2 unterstützt der gemeinsame
Identitätskatalog Mini 0.8/0.9 und Weather 0.2/0.3 sowie Mainboard 0.3.
Die tatsächliche Revision wird ausdrücklich gewählt. Das Tester-Provisioning
verwendet dieselben Definitionen und Bilder für die passende Boardansicht.
Siehe [BoardRevisionCompatibility.md](BoardRevisionCompatibility.md).
