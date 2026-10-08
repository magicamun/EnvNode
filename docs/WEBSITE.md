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

Aufgenommen: Mini, Weatherstation, ORing Power, AnalogHydroPressure, DuoRelay,
gewöhnliche unterstützte I²C-Sensoren und Firmware. Mini, Weatherstation und ORing Power sind
ausgearbeitet; die anderen Bereiche enthalten bewusst nur eine Übersicht.
Weitere Objekte und öffentliche WIP-Seiten benötigen eine Einzelfallentscheidung.
Eine WIP-Kennzeichnung macht Inhalte nicht intern.

Die Mini-Seite folgt den Rev.-0.8-Quellunterlagen. Deren offener physischer
Validierungsstatus muss noch mit dem tatsächlichen Aufbau abgeglichen werden.
Keine neue Prüfung wird aus einem historischen Prüfbericht abgeleitet.

PDFs bleiben an ihrem bisherigen Platz. Der Build-Hook bindet nur ausgewählte
Dateien ein, damit keine manuell gepflegte zweite Kopie entsteht.
Vor einer Veröffentlichung auch die Downloads inhaltlich prüfen.

## Veröffentlichung vorbereiten

Die Vorlage `.github/workflow-templates/docs-pages.yml.disabled` ist absichtlich
kein aktiver Workflow. Sie baut und verpackt die Website, enthält noch keinen
Deploy-Schritt und hat keinen Push-Trigger. Erst nach Inhaltsprüfung richten wir
GitHub Pages, den aktiven Workflow und danach `docs.envnode.de` ein.
Die Vorlage allein veröffentlicht nichts. Es wurde keine Domain gesetzt.

## Offizielle Dokumentation

- https://www.mkdocs.org/user-guide/configuration/
- https://squidfunk.github.io/mkdocs-material/getting-started/

## Schreib- und Farbregeln

Produktseiten beschreiben den aktuellen Stand ohne Rückblicke auf ältere Revisionen.
Ältere Stände bleiben intern oder werden bei Bedarf ausdrücklich als Legacy geführt.
Die aktuelle Revisionsangabe bleibt zur Zuordnung der Anleitung erhalten.
Website, Shop und PDFs verwenden die Palette aus `HardwareDocumentationStyle.md`.
Tabellen, Fließtext, Nebeninformationen und Informationsfelder folgen denselben
Farbrollen; Warnungen behalten ihre semantischen Farben.
