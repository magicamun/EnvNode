<!-- SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0 -->
# ORing Power: Prüfpunkte an den Randpads

Draufsicht auf die Bestückungsseite, USB-C unten. Aus dem unveränderten PCB-Stand
0.1.1 mit KiCad 10.0.5 gerendert, mit separaten Dokumentationsmarkierungen.
`ORing-Power-testpoints.png` ist die in Website und PDF verwendete Abbildung;
`ORing-Power-top.png` ist die native transparente Renderer-Ausgabe.
Die vorherige Vorschau bleibt zur Referenz erhalten.

Die fünf U2-Pads wurden mit der aus dem vollständigen Schaltplan exportierten
Netzliste abgeglichen. `provenance.json` dokumentiert Quellpfad und PCB-Prüfsumme.
Keine KI-generierten oder ersetzten Geometrien. Für J1 ist das referenzierte
STEP-Modell nicht installiert; deshalb wird nur der tatsächliche Footprint gezeigt.
Die Randpads, Kontur und übrigen vorhandenen Bauteilmodelle werden direkt aus
KiCad verwendet. Die Kreise markieren zugängliches Kupfer neben dem Halbloch.

`render_testpoints.py` erzeugt die beschriftete Ansicht aus dem vorhandenen
Rohbild (Python mit Pillow, macOS Arial). Der native Renderbefehl steht im Skript.
Die Pixelzuordnung wurde am tatsächlichen Konturrechteck kalibriert und visuell
geprüft; bei Änderungen an Kamera oder Layout erneut prüfen.
`tools/update_oring_testpoint_appendix.py` aktualisiert die zusätzliche PDF-Seite
und behält die ursprünglichen elf Anleitungsseiten bei.

EnvNode-eigene Dateien unterliegen [LICENSE](../../../LICENSE),
LicenseRef-EnvNode-Use-and-Redistribution-1.0. Die installierten KiCad-Modelle werden
nicht mitverteilt und behalten ihre eigenen Lizenzen. Originale Modellhinweise
stehen in `THIRD_PARTY_NOTICES.txt`.
