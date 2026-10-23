# Hardware documentation status

Audit completed 6 October 2026. This inventory covers assembly, bring-up and
jumper PDFs for implemented boards. Library copies, connector/design blocks,
mechanical templates and Empty module templates are not separate missing
assembly manuals. Archived Revision-0.1 boards remain historical designs.

## Current guides

| Board | Current PCB revision | PDF status |
| --- | --- | --- |
| EnvNode Mini | 0.8 | Rev. 0.8 available; palette and PCB views checked |
| Weatherstation | 0.2 | New Rev. 0.2 guide; connector mappings and C1 supply corrected |
| AnalogHydroPressure | 0.5 | New Rev. 0.5 guide; AUX_GPIO2 boost enable and startup checks |
| DuoRelay | 0.3 | New Rev. 0.3 guide; U2 / SOIC-8 EEPROM and assembly details |
| ORing Power | No formal PCB title-block revision | Existing Rev. 0.1 documentation refreshed; current source hashes and corrected D1/D2 assignment |
| I2C Carrier HalfSize | 0.1 | Rev. 0.1 jumper guide available; palette and PCB view checked; no source hash in guide |
| RainDetectorController | 0.3 | New Rev. 0.3 guide; current 2x2 sockets, dimensions, interfaces and bring-up |
| RainDetectorHead | 0.1 | New Rev. 0.1 guide; heater network, 2x2 headers, fabrication and test requirements |
| Legacy Raindetector | 0.2 | Matching Rev. 0.2 guide available; separate from the Controller/Head projects |

New guides are in each board's `Documentation/` directory. Older PDFs remain
historical and retain their own source revisions. The new guides contain source
hashes; ORing retains its reference rendering because the source differences do
not change geometry or circuitry. See [visual style](HardwareDocumentationStyle.md)
for the palette and camera settings.

## Missing guides

| Board | PCB title-block revision | Remaining work before/documented in a new PDF |
| --- | --- | --- |
| SX1262-Remote | 0.1 | Create guide; clarify whether README's Revision 0.3 names the interface or board revision |
| SolarRadiation | 0.1 | Create guide from an agreed hardware snapshot; local PCB/project edits are outside this documentation commit |
| RemoteNode | Not specified | Establish revision and documented scope, then create guide |

No PDF is generated for these boards without selecting it with the user.

## Verification limits and outstanding hardware work

- PDF pages were rendered and visually inspected. Recent guides were checked
  against exported schematic netlists and PCB pad nets; that is not a full
  schematic/PCB parity or design-rule check.
- No new complete ERC/DRC or physical hardware testing was performed for this
  documentation work. Historical check dates are labeled as such.
- RainDetector's 14 September ERC/DRC results predate the 15 September connector
  and outline changes. Revalidate both projects before manufacture.
- RainDetector connector mating height, real heater behavior, coating and via
  water exclusion still need qualification. The Controller CSV was regenerated
  because it still listed the previous 1x4 sockets.
- On both EnvNode Mini and Weatherstation, C1 (100 nF) decouples +5V to GND.
  The identity EEPROM is supplied from +3V3_SYS without a separate local
  decoupling capacitor. This is the same arrangement on both mainboards.
- SCT013Head REVIEW.md lists an R1 article-list mismatch and reference/DRC
  findings; inspect the current sources before carrying any historical finding
  into a new guide.
- Assembly-guide availability does not imply production approval.

## Mainboard rendering refresh — 18 October 2026

Mini and Weather assembly guides now document PCB patches 0.8.1 and 0.2.1,
respectively, with current KiCad renderings, source SHA-256 fingerprints and
document date 18 October 2026. Existing download filenames are retained.
Board Identity stays at 0.8 / 0.2. Historical ERC/DRC records remain historical;
no new validation run is implied. See [render provenance](../images/mainboards/README.md).

## SCT013Head Markdown guide - 23 October 2026

The [Rev. 0.1 assembly guide](site/sensors/sct013-head/assembly.md) now uses the
shared Markdown/front-matter schema and website theme. It is linked from the
assembly index, sensor overview, SCT013 page, compatibility matrix and navigation.
No standalone PDF is generated or registered for this initial guide.

The InvenTree article pool was checked directly: R1-R4 ENP-00018 (four 10-kΩ
resistors), C1 ENP-00044, C2 ENP-00007, pigtail ENP-00127, socket ENP-00125,
PCB ENP-00126. This is an article assignment, not a stock-availability check.
The guide supersedes the historical 1-kΩ R1 assembly note and identifies ADC1
as solder pads for the pigtail. It covers assembly without the socket/sensor,
unpowered continuity checks, bias measurements and raw ADC noise captures.

The user reviewed the local themed HTML preview. Source hashes and real KiCad
views are included. Physical testing and socket fit remain open. The attempted
new DRC run aborted without a report; no successful DRC result is claimed.
Local `mkdocs serve` skips PDF rendering and serves existing reference PDFs;
production `mkdocs build` retains mandatory PDF generation and validation.
