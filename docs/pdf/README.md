# PDF-Pilot: DuoRelay 0.3

Einzige Inhaltsquelle: `docs/site/modules/duo-relay/assembly.md`.
Die letzte historische PDF mit Stand 17.10.2026 bleibt unverändert.
Bilder wurden unverändert aus deren Seiten 4 und 5 übernommen.

## Build

Python-Umgebung erstellen und `pip install -r requirements-docs.txt` ausführen.
WeasyPrint benötigt Pango: macOS `brew install pango`; Ubuntu siehe docs.yml.
Dann im Repository-Root:

```sh
python -m mkdocs build --strict
```

Der Build erstellt zunächst HTML, entnimmt genau dessen Artikelinhalt und erzeugt
anschließend automatisch `site/assets/downloads/DuoRelay_Rev0.3_Markdown-Pilot.pdf`.
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
