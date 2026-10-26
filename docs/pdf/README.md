# PDF-Erstellung: aktuelle Aufbauanleitungen

Jede Anleitung besitzt genau eine redaktionelle Markdown-Quelle. Die Auswahl der acht aktuellen Anleitungen steht in `documents.yml`. Die ursprünglichen PDFs bleiben unverändert, ihre Ansichten werden als Inhaltsbilder übernommen.

## Build

Python-Umgebung erstellen und `pip install -r requirements-docs.txt` ausführen.
WeasyPrint benötigt Pango: macOS `brew install pango`; Ubuntu siehe docs.yml.
Dann im Repository-Root:

```sh
python -m mkdocs build --strict
```

Der Build erstellt zunächst HTML, entnimmt genau dessen Artikelinhalt und erzeugt
anschließend automatisch alle registrierten PDFs unter `site/assets/downloads/`, zum Beispiel `DuoRelay_Rev0.3_Anleitung.pdf`.
Daneben entsteht ein JSON-Buildnachweis. Keine gesonderte Markdown-Konvertierung
und kein manuelles PDF-Nachbearbeiten. Die ursprüngliche PDF wird nicht ersetzt.

`template.html` ist die feste Umschlagvorlage, `print.css` definiert A4, Umbrüche,
Tabellenköpfe, Bilder sowie Kopf-/Fußzeilen. Farben werden zwingend aus den
`--envnode-*`-Definitionen der Website geladen; unvollständige Palette bricht ab.
Druckvorlagenänderungen und der Generator sind im CI-Pfadfilter enthalten.
Schriftdateien DejaVuSans.ttf, DejaVuSans-Bold.ttf und DejaVuSansMono.ttf werden verbindlich eingebettet.
Linux-Standard: /usr/share/fonts/truetype/dejavu; lokal ENVNODE_PDF_FONT_DIR setzen.
Fehlende Fonts brechen den Build ab statt einen Ersatzfont zu verwenden.

Vor Buildabschluss ist der generierte Download nur reserviert. Ein PDF-Fehler
bricht den gesamten Build ab; CI lädt dann kein Pages-Artefakt hoch. Nur vollständige
Builds veröffentlichen. Die Ausgabe wird auf A4 und zentrale Inhalte geprüft.
Zusätzlich ist visuelle Seitenprüfung nach Vorlagen- oder wesentlichen Inhaltsänderungen nötig.
Versionsgleichheit bedeutet reproduzierbaren Prozess, keine Garantie bytegleicher PDFs
zwischen unterschiedlichen Font-/Pango-Systemversionen. JSON protokolliert die Eingaben.

Metadaten `pdf:` am Markdown-Anfang bestimmen Hardware- und Dokumentversion,
Dokumentdatum, Status und ursprünglichen Quelldokumentstand. Diese Angaben bewusst
pflegen, nicht automatisch als neues Hardware-Prüfdatum ausgeben.

## Auswahl und Pflege der Anleitungen

`documents.yml` registriert explizit jede Markdown-Quelle, die ursprüngliche letzte PDF, das Bildverzeichnis, den neuen PDF-Ausgabepfad und inhaltliche Pflichtmerkmale. Es gibt keine automatische Auswahl nach Dateinamen oder Revisionsnummer. Historische PDFs und Herstellerdatenblätter werden nicht migriert.

Die zentrale Webübersicht liegt unter `docs/site/guides/index.md`. Änderungen werden künftig in der jeweiligen `assembly.md` gepflegt; beim MkDocs-Build entstehen HTML, PDF und JSON-Nachweis gemeinsam. Keine separate redaktionelle PDF-Quelle pflegen.

Neue Anleitung: Markdown mit `pdf`-Metadaten anlegen, einen geprüften Eintrag in `documents.yml` ergänzen und in Übersicht sowie Produktseite verlinken. Anschließend `mkdocs build --strict` und die neu gerenderten PDF-Seiten prüfen. Die Manifestdatei wird von den bestehenden CI-Pfadfiltern `docs/pdf/**` erfasst.
