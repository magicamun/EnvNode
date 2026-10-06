# Hardware documentation appearance

Assembly, bring-up and jumper PDFs use the EnvNode shop palette. The reference
is https://www.thielemann-net.de/, inspected on 27 September 2026. The shop was
unavailable during the follow-up audit on 5 October 2026; that audit therefore
uses the previously verified palette.

| Role | Color |
| --- | --- |
| Headings, table headers, primary accents | `#9E6C00` |
| Accent rules and diagram pads | `#CC9319` |
| Cover accent | `#CA8E01` |
| Body text | `#262626` |
| Secondary text | `#595959` |
| Table rules | `#D6D6D6` |
| Neutral alternating rows | `#F5F5F5` |
| Information panels | `#FAF4E6` |
| Page background / reversed text | `#FFFFFF` |

Red and orange safety warnings retain their semantic colors. Solder bridges
in jumper diagrams use dark strokes so they remain distinct from gold pads.
Table headers with dark text use the pale gold background instead of a dark
fill; white header text uses the dark gold background for sufficient contrast.
PCB materials and component colors are physical render colors, not document
branding colors. Supplier datasheets and third-party documents are excluded.

## PCB views

The visual reference is `ORing_Power_3D.png` in the ORing Power design block's
Documentation directory. Keep that reference unchanged. Use a shallow front
view with a small sideways rotation, rather than a strongly diagonal view.

The following KiCad CLI settings reproduce a comparable view consistently:

```sh
kicad-cli pcb render --width 2000 --height 2000 \
  --side top --background transparent --quality high \
  --rotate '-25,0,5' --perspective --zoom 0.72 \
  --output board-top.png board.kicad_pcb

kicad-cli pcb render --width 2000 --height 2000 \
  --side bottom --background transparent --quality high \
  --rotate '-25,0,-5' --perspective --zoom 0.72 \
  --output board-bottom.png board.kicad_pcb
```

The bottom-view rotation compensates for viewing the opposite side. Do not
mirror the rendered image. Keep connector orientation and board labeling
faithful to the PCB data.

Reduce `--zoom` to `0.6` for DuoRelay and the I2C Carrier to keep their complete
shadows inside the render canvas. This changes framing, not the viewing angle.

Render the PCB revision documented by the PDF, not automatically the latest
working-tree revision. Prefer an exact match to the PDF's PCB SHA-256. Crop
transparent margins, retain all geometry and shadows, and fit the resulting
image proportionally inside the PDF's existing image area. Never independently
scale image width and height. When replacing a PDF image, regenerate its image
object; do not retain a transparency mask from the previous rendering.

## Audit of 5 October 2026

Twelve project PDFs (108 pages) were checked. Mini Rev. 0.8 and the I2C Carrier
guide needed palette corrections. Mini Rev. 0.6 also needed gold headings and
legible table headers in place of dark text on a dark fill. Ten PDFs received refreshed PCB views;
ORing Power remained the reference and Mini Rev. 0.3 contains no PCB rendering.

| Guide | PCB source commit | Source verification |
| --- | --- | --- |
| Mini Rev. 0.2 | `31e7549f` | Exact documented PCB SHA-256 |
| Mini Rev. 0.6 | `1460b8e9` | Release commit and PCB revision 0.6 |
| Mini Rev. 0.8 | `a388da24` | Exact documented PCB SHA-256 |
| Weather Rev. 0.1 | `ba32008d` | Release commit and PCB revision 0.1 |
| AnalogHydroPressure Rev. 0.4 | `f4962bb4` | Exact documented PCB SHA-256 |
| DuoRelay Rev. 0.2 | `03c1ba46` | Exact documented PCB SHA-256 |
| Mainboard Rev. 0.1 | `e8917307` | Exact documented PCB SHA-256 |
| RainDetector Rev. 0.2 | `42cc4c58` | Exact documented PCB SHA-256 |
| RainDetector Rev. 1 | `cde3f56f` | Exact documented PCB SHA-256 |
| I2C Carrier Rev. 0.1 | `67ca736` | Routed revision 0.1 with the documented jumper correction |

Validation preserves text, page count, page dimensions and all PDF drawing
operations except the intended color substitutions and image placement
matrices. Render the resulting pages to check image clipping, proportions,
legibility and layout before publishing.
