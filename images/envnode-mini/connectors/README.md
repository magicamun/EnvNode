<!-- SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0 -->
# EnvNode Mini: Anschlussdetails ADC1 und J5

Erstellt am 18.10.2026 aus dem aktuellen Arbeitsstand von
`hardware/kicad/MainBoards/EnvNode Mini/EnvNode Mini.kicad_pcb` mit KiCad 10.0.5.
Dieser PCB-Stand enthielt bereits lokale Änderungen. Quell- und Modellprüfsummen
stehen in `provenance.json`; die Quelldateien wurden nicht verändert.

## Abbildungen

- `ADC1-oblique-annotated.png` / `.svg`: JST-PH, räumliche Detailansicht mit Pin 1.
- `ADC1-top-annotated.png` / `.svg`: Draufsicht mit sichtbarer Referenz und Signalbeschriftung.
- `J5-oblique-annotated.png` / `.svg`: JST-SH, räumliche Detailansicht mit Pin 1.
- `J5-top-annotated.png` / `.svg`: Draufsicht mit sichtbarer Referenz und Signalbeschriftung.
- Die gleichnamigen PNGs ohne `-annotated` sind unveränderte KiCad-Renderausgaben.

Die beschrifteten PNGs sind 2000 × 2310 Pixel groß. SVGs enthalten die originale
Rasteransicht eingebettet und separat editierbare Texte und Pinmarkierungen;
die 3D-Geometrie ist im SVG kein Vektormodell. Orange Markierungen und Kopf-/Fußtexte
sind Dokumentationsgrafik, kein auf die Platine hinzugefügter Bestückungsdruck.

## Verifizierte Pinbelegung

Die nummerierten Footprint-Pads wurden mit einer frisch aus dem vollständigen
Mini-Schaltplan exportierten Netzliste verglichen. Alle acht Anschlüsse stimmen überein.

| Pin | ADC1: JST-PH B4B-PH-K, 2,00 mm | J5: JST-SH BM04B-SRSS-TB, 1,00 mm |
|---|---|---|
| **1** | **GND** | **GND** |
| 2 | +5V | +3V3_SYS |
| 3 | /ADC/ADC1 | I2C0_SDA |
| 4 | +3V3_SYS | I2C0_SCL |

Blick auf die **Bestückungsseite**, ohne Spiegelung: ADC1 ist in der Draufsicht
von oben nach unten 1–4 nummeriert. Bei J5 verläuft die Nummerierung links nach
rechts. Die beiden seitlichen J5-Pads `MP` sind mechanische Befestigungspads,
keine weiteren Signalpins. Bei ADC1 zeigt der Kreis auf den ersten Steckkontakt;
bei J5 auf dessen zugänglichen ersten Lötanschluss.

Tatsächlicher Silkscreen: ADC1 trägt `GND`, `+5V`, `GPIO`, `3.3V`;
`GPIO` entspricht hier dem Netz `/ADC/ADC1`. J5 trägt `GND`, `3V3`, `SDA0`, `SCL0`.
In der Schrägansicht verdeckt das J5-Gehäuse die Referenz teilweise bzw. vollständig;
die ergänzende Draufsicht zeigt `J5` an der tatsächlichen Position.

## Geometrie und Ausschnitte

Es werden ausschließlich die im PCB referenzierten STEP-Modelle mit ihren
hinterlegten Transformationen verwendet. Keine KI-generierte, ersetzte oder
nachgezeichnete Steckergeometrie. Originaler Silkscreen, Kupfer und Padgeometrie
werden durch KiCad gerendert.

Für die Makroansichten erstellt das Skript temporäre PCB-Ableitungen mit rechteckiger
virtueller Schnittkontur. Außerhalb liegende Footprints werden weggelassen;
Footprints im Ausschnitt, deren STEP-Transformationen und PCB-Grafiken bleiben erhalten.
Die rechte Konturkante bei X = 192,372 mm entspricht der echten geraden Boardkante;
die anderen drei geraden Kanten sind **virtuelle Schnittkanten**, keine Aussage
über die tatsächliche Gesamtplatinenform. Ausschnitte in PCB-Koordinaten (mm):

- ADC1: X = 177 … 192,372; Y = 112 … 127.
- J5: X = 182 … 192,372; Y = 94 … 107.

Die Renderings verwenden die im PCB gespeicherten Kupferfüllungen; es wird keine
neue Fertigungsprüfung oder Zonenfüllung durchgeführt. Die virtuellen Schnitte
können angeschnittene Leiterbahnen am Rand zeigen. Das ist eine Dokumentationsansicht,
kein eigenständiges herstellbares PCB. Farben und Licht sind Renderer-Darstellung,
keine Spezifikation der realen Materialfarben.

## Reproduzieren

KiCad 10 CLI und Python mit Pillow werden benötigt:

```sh
python3 images/envnode-mini/connectors/render.py
```

Bei abweichender Installation `KICAD_CLI` und `KICAD10_3DMODEL_DIR` setzen.
`--annotate-only` erzeugt die Overlays erneut aus vorhandenen Rohbildern.
Die Pinbelegung wird vor dem Export gegen PCB und Schaltplan geprüft.
Die Overlaypositionen sind auf die gespeicherte Kamera und die beiden aktuellen
Footprints abgestimmt; nach Layout- oder Modelländerungen visuell neu prüfen.
Schriftart für PNGs: macOS Arial; SVGs verwenden Arial mit Sans-Serif-Fallback.

## Lizenz und Modellnachweise

EnvNode-eigene Abbildungen, Dokumentation und Skript unterliegen der
[Root-Lizenz](../../../LICENSE), `LicenseRef-EnvNode-Use-and-Redistribution-1.0`.
EnvNode ist source-available. Die eingebundenen KiCad-Modelle behalten ihre eigene
Lizenz CC BY-SA 4.0 mit der KiCad-Ausnahme für elektronische Designs und daraus
generierte Dateien. Die Bibliotheksmodelle selbst werden hier nicht mitverteilt.

- PH-Modell: Copyright (C) 2017, Rene Poeschl.
- SH-Modell: Copyright (C) 2025, KiCAD.

Originale Modellhinweise: [THIRD_PARTY_NOTICES.txt](THIRD_PARTY_NOTICES.txt).
