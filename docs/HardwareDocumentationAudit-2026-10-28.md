# Dokumentationsinventur – 28. Oktober 2026

> Historische Basisinventur vor der anschließenden Bearbeitung. Der Projektinhaber hat die sieben ausgearbeiteten Anleitungen am 28.10.2026 als **abgeschlossen** freigegeben. Neue Revisionen und Anleitungen sind inzwischen erstellt; maßgeblich ist der [aktuelle Statusbericht](HardwareDocumentationStatus.md). Die nachfolgenden damaligen Einstufungen dokumentieren den Ausgangsstand.


Geprüft wurden die Dokumentationsdateien in `main` und ergänzend die vorhandenen
Arbeitszweige `analog-hydro-pressure`, `sd-card`, `SX1262-Remote`,
`test-provision-board` und `rain-detector`. Dies ist eine Bestands- und
Abschlussprüfung, kein Abgleich mit der neuesten Hardware und keine technische
Freigabe. Die Live-Website war über das Webwerkzeug nicht erreichbar; „Web“
bezeichnet hier die Websitequellen in `main`, nicht einen bestätigten Deploymentstand.

## Bedeutung der Zustände

- **Ausgearbeitet:** Bestückung, Anschlüsse, Inbetriebnahme/Prüfablauf und
  Abschluss-/Fehlerhinweise liegen vor. Ein expliziter redaktioneller
  Freigabevermerk fehlt; „abgeschlossen/freigegeben“ ist daher nicht belegt.
- **Teilumfang:** Eine Anleitung existiert, deckt aber ausdrücklich nur einen
  Teil des Aufbaus oder Tests ab.
- **Entwurf:** Das Dokument bezeichnet sich selbst als Entwurf.
- **Fehlt:** Keine eigenständige Bestückungs-/Bring-up-Anleitung gefunden.
  Ein README oder CAD-Prüfbericht allein ersetzt sie nicht.

Ausstehende Hardwaretests machen eine ausgearbeitete Anleitung nicht automatisch
unvollständig. Umgekehrt ist eine vorhandene PDF-Datei kein Abschlussnachweis.

## Board- und Revisionsbestand

| Board | Dokumentierte Revision(en) | Dokumentationsstand | Format / Ablage | Noch offen an der Dokumentation |
| --- | --- | --- | --- | --- |
| EnvNode Mini | 0.8.1 (Dateiname der Referenz: 0.8) | Ausgearbeitet | Web + generierte PDF; historische Referenz-PDF | Redaktionellen Abschluss ausdrücklich festhalten |
| EnvNode Mini, älter | 0.6, 0.3, 0.2 | Historische Anleitungen vorhanden | PDF im Boardverzeichnis; jetzt Archivlinks | Keine heutige Freigabe oder Überarbeitung aus dem Vorhandensein abgeleitet |
| Weatherstation | 0.2.1 (Referenzdatei: 0.2) | Ausgearbeitet | Web + generierte PDF | Redaktionellen Abschluss ausdrücklich festhalten |
| Weatherstation, älter | 0.1 | Historische Anleitung vorhanden | PDF; jetzt Archivlink | Historischer Stand |
| ORing Power | 0.1 mit Ergänzung 0.1.1 | Ausgearbeitet | Web + generierte PDF + Referenz-PDF | Keine eigenständige ältere Anleitung; 0.1 und 0.1.1 nicht als zwei vollständige Anleitungen zählen |
| DuoRelay | 0.3 | Ausgearbeitet | Web + generierte PDF | Redaktionellen Abschluss ausdrücklich festhalten |
| DuoRelay, älter | 0.2 | Historische Anleitung vorhanden | PDF; jetzt Archivlink | Historischer Stand |
| AnalogHydroPressure | 0.5 | Ausgearbeitet | Web + generierte PDF in main | Redaktionellen Abschluss ausdrücklich festhalten |
| AnalogHydroPressure, älter | 0.4 | Historische Anleitung vorhanden | PDF; jetzt Archivlink | Historischer Stand |
| AnalogHydroPressure | 0.6 | Entwurf, 12 Seiten, 22.10.2026 | PDF + Review im Arbeitszweig analog-hydro-pressure | Redaktionell abschließen und bewusst in Website/main übernehmen; bleibt bis dahin außerhalb des öffentlichen Archivs |
| RainDetectorController | 0.3 | Ausgearbeitet, Entwicklungsstand | Web + generierte PDF | Redaktionellen Abschluss ausdrücklich festhalten |
| RainDetectorHead | 0.1 | Ausgearbeitet, Entwicklungsstand | Web + generierte PDF | Redaktionellen Abschluss ausdrücklich festhalten |
| SCT013Head | 0.1, Dokumentversion 1.1 vom 24.10.2026 | Teilumfang: Erstaufbau/Nullmessung beschrieben | Web/Markdown; keine registrierte PDF-Ausgabe | Sensorbetrieb und Buchsenmechanik ergänzen; PDF-Ausgabe bei Bedarf registrieren |
| I²C Carrier HalfSize | 0.1 | Teilumfang: Montage und Jumper | Web + generierte PDF + Referenz-PDF | Vollständigen elektrischen Bring-up ergänzen, falls als vollständige Boarddokumentation gewünscht |
| SD-Card | 0.1 | Erster Entwurf, 11 Seiten, 20.10.2026 | PDF + README im Arbeitszweig sd-card; keine Boardseite in main | Anleitung redaktionell abschließen, neuere im README dokumentierte Ergebnisse einarbeiten und Webfassung erstellen |
| SX1262-Remote | PCB laut bisheriger Inventur 0.1; README nennt 0.3 | Fehlt | Technisches README vorhanden, auch im gleichnamigen Arbeitszweig keine Aufbauanleitung gefunden | Revisionsbezug festlegen; Bestückung, Anschlüsse, Bring-up und Fehlerbilder erstellen |
| SolarRadiation | 0.1 laut bisheriger Inventur | Fehlt | KiCad-Projekt, keine Aufbauanleitung gefunden | Eigenständige Anleitung erstellen |
| RemoteNode | Nicht festgelegt | Fehlt | KiCad-Projekt, keine Aufbauanleitung gefunden | Revisions- und Dokumentationsumfang festlegen, Anleitung erstellen |
| Test-Provisioning (monolithisch) | Entwicklungsstand | Fehlt / Projekt abgelöst im Arbeitszweig | README in main; im Arbeitszweig durch drei Projekte ersetzt | Keine neue Anleitung für das abgelöste Design einplanen, solange es nicht weiterverwendet wird |
| Test Controller | 0.1 | Teilumfang: technische Beschreibung | README im Arbeitszweig test-provision-board | Bestückung, sicherer Gesamtablauf und Prüfprotokoll fehlen als vollständige Anleitung |
| Input Adapter | 0.1 | Teilumfang: Anschluss- und Routingbeschreibung | README im Arbeitszweig test-provision-board | Bestückungs-/Bring-up-Anleitung und Gesamttest ergänzen |
| Module Adapter | 0.1 | Teilumfang: Anschlussbeschreibung | README im Arbeitszweig test-provision-board | Bestückung, Orientierungskontrolle und dokumentierten Durchgangstest ergänzen |
| EnvNode Mainboard (Revision-0.1) | 0.1 | Historische Anleitung vorhanden | PDF; separates historisches Boardprojekt | Keine neue redaktionelle Freigabe geprüft |
| Ursprünglicher Raindetector | 0.2 und Dateibezeichnung Rev1 | Historische Anleitungen vorhanden | Zwei PDFs; separates historisches Sensorprojekt | Rev1 nicht durch numerische Sortierung als aktuellen Stand auswählen |

Die Tabelle nennt nachgewiesene Dokumentstände. Aus Lücken in Revisionsnummern
folgt nicht, dass jede Zwischenrevision eine eigene Anleitung benötigt.
Empty-SMD/THT-Vorlagen, Bibliothekskopien, DesignBlocks und mechanische Vorlagen
zählen nicht als Boards mit fehlender eigenständiger Aufbauanleitung.

## Ergebnis und nächste redaktionelle Schritte

Sieben aktuelle Anleitungen in `main` sind im Aufbau-/Bring-up-Umfang ausgearbeitet:
Mini, Weatherstation, ORing Power, DuoRelay, AnalogHydroPressure 0.5,
RainDetectorController und RainDetectorHead. **Ein ausdrücklich dokumentierter
redaktioneller Abschluss ist damit nicht belegt.**

Zwei weitere veröffentlichbare Anleitungen haben begrenzten Umfang: SCT013Head und
I²C Carrier. Zwei zusätzliche PDF-Entwürfe liegen ausschließlich in Arbeitszweigen:
AnalogHydroPressure 0.6 und SD-Card 0.1. Für SX1262-Remote, SolarRadiation und
RemoteNode fehlen eigenständige Anleitungen. Die neue dreiteilige Testhardware
besitzt technische READMEs, aber noch keine vollständigen Aufbauanleitungen.

Priorität für den redaktionellen Abschluss:

1. Bei den sieben ausgearbeiteten Anleitungen Abschluss/Review und Dokumentversion
   ausdrücklich festhalten. Hardware-Freigabe davon getrennt führen.
2. AnalogHydroPressure 0.6 und SD-Card 0.1 als Entwürfe abschließen und ihre
   Übernahme in main/Website entscheiden. Kein automatisches Hochstufen allein
   aufgrund einer höheren Revisionsnummer.
3. SCT013Head um den vollständigen Sensorbetrieb, I²C Carrier um den vollständigen
   Bring-up ergänzen, soweit dies zum gewünschten Dokumentationsumfang gehört.
4. Fehlende Anleitungen für Funkmodul, SolarRadiation, RemoteNode und die drei
   neuen Testboards erstellen.

## Neue Websiteablage

Die bestehenden Boardadressen von Mini, Weatherstation, ORing Power, DuoRelay
und AnalogHydroPressure zeigen direkt die ausgearbeitete Anleitung. Die bisherigen
Produktübersichten bleiben unter `overview/` erreichbar. Die früheren
`assembly/`-Adressen enthalten einen Wegweiser zur Boardadresse.
Die PDF-Erzeugung verwendet weiterhin dieselbe Markdownquelle wie die Webanleitung.
Sensor- und I²C-Carrier-Anleitungen behalten ihre vorhandenen direkten Adressen.

Pro Board gibt es eine Liste `old-revisions.md`; zusätzlich führt
`guides/old-revisions.md` alle vorhandenen historischen Anleitungen zusammen.
Das Archiv verwendet ausdrücklich ausgewählte Original-PDFs, keine automatische
Revisionssortierung. Bestehende Download-URLs bleiben erhalten. Entwürfe aus
Arbeitszweigen werden durch diese Umstrukturierung nicht veröffentlicht.

## Prüfung der Umstrukturierung

- MkDocs-HTML-Build mit `strict: true` im vorgesehenen Vorschaumodus erfolgreich.
- Alle lokalen Link- und Bildziele der erzeugten HTML-Dateien vorhanden.
- Die fünf umgestellten Boardadressen enthalten die vollständigen Kapitel und
  den Link zur jeweiligen Liste „Alte Revisionen“.
- Alle acht PDF-Manifesteinträge finden ihre erzeugte HTML-Seite; sämtliche
  im Manifest geforderten Textinhalte sind dort vorhanden.
- `git diff --check` ohne Befund.
- Produktionsbuild versucht, aber vor der PDF-Erzeugung wegen fehlender lokaler
  Pango-Systembibliothek abgebrochen. PDF-Neuerzeugung und Drucklayout nach
  der Umstellung sind lokal nicht bestätigt. Der bestehende CI-Workflow
  installiert Pango und die benötigten Schriften; ein neuer CI-Lauf wurde
  im Rahmen dieser lokalen Bearbeitung nicht gestartet.
- Keine Veröffentlichung und kein Push durchgeführt.
