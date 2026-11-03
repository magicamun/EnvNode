# Dokumentationswebsite bearbeiten

Die Website verwendet ausschließlich `docs/site/` und die ausdrücklich in
`tools/docs_assets.py` aufgeführten Downloads. Bestehende technische Dokumente,
ADRs, TODOs und Hardware-Verzeichnisse werden nicht automatisch veröffentlicht.
Nicht für die Website bestimmte Entwürfe bleiben außerhalb von `docs/site/`.
Ein versteckter Navigationseintrag ist kein Veröffentlichungsfilter.

## Lokal starten (im Repository-Root)

```sh
python3 -m venv .venv-docs
.venv-docs/bin/python -m pip install -r requirements-docs.txt
.venv-docs/bin/python -m mkdocs serve -a 127.0.0.1:8000
```

Vorschau: http://127.0.0.1:8000

```sh
.venv-docs/bin/python -m mkdocs build --strict
```

## Redaktioneller Umfang

Den Board- und Revisionsbestand sowie Teilanleitungen, Entwürfe und fehlende
Dokumentation erfasst die [Inventur vom 28. Oktober 2026](HardwareDocumentationAudit-2026-10-28.md).
Vorhandene Anleitungen und abgeschlossene Hardware-Erprobung sind getrennte Zustände.

Die Boardadresse öffnet die letzte abgeschlossene vollständige Anleitung in main.
Produktübersichten liegen separat unter `overview.md`. Frühere `assembly.md`-Adressen
bleiben als Wegweiser erhalten. Teilanleitungen werden nicht als vollständig bezeichnet.
Die Hardware-Dokumentationsrunde vom 28.10.2026 übernimmt ausdrücklich Mini 0.9.0, Weather 0.3.0, AnalogHydroPressure 0.6, SD-Card 0.1, RainDetectorController 0.4, Head 0.2 und die drei Testboards 0.1. Rendernachweise nennen Arbeitszweig, Basiscommit und SHA-256 des tatsächlich dokumentierten Quellstands.

Unter **Alte Revisionen** stehen ausdrücklich ausgewählte historische Original-PDFs
als Liste, je Board und gesammelt unter `guides/old-revisions.md`.
Die Dateien bleiben an ihrem bisherigen Platz; `tools/docs_assets.py` bindet sie
über eine feste Liste ein. Keine automatische Auswahl nach Revisionsnummern.
Aktuelle und archivierte Web-Anleitungen erzeugen ihre PDFs über `docs/pdf/documents.yml` ausgewählt.
Bestehende Download-URLs bleiben erreichbar.

## Veröffentlichung vorbereiten

Der aktive Workflow `.github/workflows/docs.yml` baut und veröffentlicht die Website
bei relevanten Änderungen auf `main` oder nach manuellem Start in GitHub Actions.
GitHub Pages verwendet als Quelle GitHub Actions.
Die zunächst verwendete Adresse ist https://docs.envnode.de/.
Die eigene Domain ist in GitHub Pages hinterlegt; `site_url` verwendet dieselbe Adresse.
Es werden ausschließlich `docs/site/` und die ausgewählten Downloads veröffentlicht.

## Offizielle Dokumentation

- https://www.mkdocs.org/user-guide/configuration/
- https://squidfunk.github.io/mkdocs-material/getting-started/

## Schreib- und Farbregeln

Boardseiten führen direkt zur letzten abgeschlossenen Anleitung.
Ältere Dokumentstände stehen in der gesonderten Liste „Alte Revisionen“;
historische Boardprojekte bleiben ausdrücklich als solche gekennzeichnet.
Die aktuelle Revisionsangabe bleibt zur Zuordnung der Anleitung erhalten.
Website, Shop und PDFs verwenden die Palette aus `HardwareDocumentationStyle.md`.
Tabellen, Fließtext, Nebeninformationen und Informationsfelder folgen denselben
Farbrollen; Warnungen behalten ihre semantischen Farben.

Sensoren enthält die I²C-Treiberübersicht sowie ausdrücklich öffentliche WIP-Seiten
für RainDetector und SCT013. Firmware- und Displayangaben folgen dem am
9. Oktober 2026 geprüften Quellstand; Treiberlisten bei Änderungen abgleichen.

Aktuelle PCB-Stände und Downloadnamen werden im PDF-Manifest und den Boardseiten
ausdrücklich gepflegt: Mini 0.9.0, Weather 0.3.0, ORing Power 0.1.1,
AnalogHydroPressure 0.6, SD-Card 0.1, RainDetectorController 0.4, Head 0.2
und die Testboards 0.1. Alte Patchnamen und ursprüngliche Download-URLs bleiben
für die zugehörigen historischen Revisionen erreichbar; sie dürfen nicht auf
eine andere Hardware-Revision umgebogen werden. ORing enthält weiterhin die
Basisanleitung 0.1 samt Ergänzung für 0.1.1.

### Lokale HTML-Vorschau ohne PDF-Systembibliotheken

`mkdocs serve` verwendet dasselbe Website-Theme und dieselben Stylesheets wie
`mkdocs build`, überspringt aber die PDF-Neuerzeugung. Dafür sind Pango und
DejaVu-Systemschriften nicht erforderlich. PDF-Links zeigen in dieser lokalen
Vorschau die bereits vorhandenen Referenz-PDFs, nicht die aktuelle Druckausgabe
der Markdown-Seiten. Der Start meldet diesen Unterschied ausdrücklich.

`mkdocs build` (auch für die Veröffentlichung) erzeugt weiterhin alle registrierten
PDFs und bricht bei fehlenden Bibliotheken oder fehlerhafter PDF-Erzeugung ab.
Der Vorschaumodus validiert daher das Weblayout, nicht das PDF-Drucklayout.

### Neue und archivierte PDF-Quellen

`docs/pdf/documents.yml` wählt jede Quelle und jeden Zielnamen ausdrücklich aus.
`archive: true` kennzeichnet erhaltene historische Markdown-Anleitungen.
`original` ist nur bei aus früheren PDFs übernommenen Quellen erforderlich;
neue Web-Anleitungen haben keine historische PDF-Basis.
`preview` nennt die geprüfte lokale PDF-Referenz unter `docs/pdf/reference/`.
Nach einer inhaltlichen Änderung zuerst den Produktionsbuild ausführen, die Ausgabe
visuell prüfen und die zugehörige Referenz-PDF samt JSON-Nachweis aktualisieren.
Der Vorschaumodus bricht bei fehlender Referenz verständlich ab und liefert keine
leeren PDF-Dateien aus. Alte Downloadnamen bleiben erhalten.

Der redaktionelle Status wird unter `guides/status.md` öffentlich geführt.
Firmware-Releases und Binärabbilder sind als eigener Folgeschritt vorgemerkt.

### 3D-Einstieg und Bestückungspläne

Alle gepflegten Anleitungen einschließlich der archivierten Markdownfassungen
folgen der [Bildfolge](HardwareDocumentationStyle.md): 3D-Vorderseite vor der
Teileliste; 3D-Rückseite nur bei rückseitiger Bestückung. Die Bestückungsübersicht
verwendet senkrechte farbige KiCad-Renderings ohne Bauteilmodelle.

Die Bildquellen stehen explizit in `docs/board-views.yml`. Zur Neuerzeugung
`requirements-docs-render.txt` installieren und `python tools/docs_board_views.py`
ausführen; `--id mini-0.9.0` wählt einen einzelnen Stand. Die Quellprüfsummen
verhindern unbemerkte Aktualisierungen auf eine andere PCB-Version. Der normale
Websitebuild verwendet die eingecheckten PNG-Dateien und benötigt weder
KiCad noch die zusätzlichen Render-Abhängigkeiten.

`guides/board-views.md` listet die Downloads; `assets/board-views/index.json`
und `views.json` pro Board machen die Vorderseiten insbesondere für die
Testsoftware wiederverwendbar. Bauteilpositionen sind in KiCad-Millimetern
dokumentiert; eine Pixelabbildung muss für das zugeschnittene Rendering kalibriert werden.
