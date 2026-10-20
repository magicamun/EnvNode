# EnvNode SD-Card Rev. 0.1

EnvNode-owned materials: LicenseRef-EnvNode-Use-and-Redistribution-1.0; see the
repository root LICENSE. Adafruit designs/adapters and KiCad models retain their
respective licenses; see `../../../Libraries/Adafruit-Breakouts/`.

FullSize carrier (38 x 64 mm) for the assembled **Adafruit MicroSD card breakout
board+ / product 254** (Reichelt DEBO MICROSD). J1 is the bottom-mounted 2x7
EnvNode module connector. U1 mounts on the top through a vertical 1x8, 2.54 mm
header soldered directly to the carrier; choose its height and supports together. U2 (24LC32,
SOIC-8) and C1 (100 nF, 1206) are on the bottom; R1 (10 kOhm, 1206) is on top.

## Purpose and scope

Optional field recording for Raindetector validation in real rain. Measurements
and diagnostics are evaluated externally. Normal measurement, MQTT and heater
control must remain functional without an SD card or on a storage error. This
hardware does not provide weather analysis, historical aggregation or a finished
firmware logging subsystem.

## Connections

| U1 pin | Function | J1 / carrier connection |
| --- | --- | --- |
| 1 | 5V_IN | J1.4, +5V |
| 2 | 3V3_REG | Deliberately unconnected; onboard regulator output |
| 3 | GND | J1.1 |
| 4 | CLK | J1.13, SPI_SCK |
| 5 | DO / MISO | J1.12, SPI_MISO |
| 6 | DI / MOSI | J1.11, SPI_MOSI |
| 7 | CS | J1.14, slot-specific SPI_CS |
| 8 | CD | J1.9, AUX_GPIO1; R1 = 10 kOhm to +3V3_SYS |

Do not join U1.2 to +3V3_SYS. I2C0 connects U2 for identification at 0x52 in
slot A or 0x53 in slot B; A1 is HIGH, A2 LOW, A0 follows J1.3. C1 decouples
+3V3_SYS locally. I2C1 and AUX_GPIO2 are unused. CD is a mechanical contact:
Adafruit documents LOW without a card and HIGH with a card; verify the actual
socket's polarity.

## Assembly and bring-up

[Bestückungs- und Bring-up-Anleitung Rev. 0.1](Documentation/SD_Card_Bestueckungs_und_Bringup_Rev0.1.pdf)

Follow the existing [module interface specification](../../../../../docs/EnvNode_Module_Interface_Hardware_Specification.md).
The overlap of U1 and H1 is intentional. Fit/check the H1 fastening before
installing U1; ensure insulated mechanical clearance beneath the breakout.
The four U1 support holes are 2.2 mm (#2 hardware), not M2.5. H1/H2 are 2.7 mm
for M2.5 hardware. Spacer dimensions must match the actual connector stack.
The model views show the carrier without U1, which has no 3D model assigned.

Local `sym-lib-table` and `fp-lib-table` point to libraries in this checkout.
The PDF can be regenerated with `tools/build_sd_card_bringup.py` using Python,
ReportLab and Pillow. Reviewed KiCad render images are kept beside the PDF;
rerender them after a PCB revision. The generator rejects stale image manifests.

## InvenTree parts - 20 October 2026

The following parts were checked against the live InvenTree catalogue. Quantities
are per module; no stock receipts, consumption or orders were booked.

| Position | Qty | Part | Description |
| --- | ---: | --- | --- |
| U1 | 1 | [ENP-00136](http://inventree.thielemann.intern/web/part/133/details) | Adafruit MicroSD card breakout board+ (254); Reichelt DEBO MICROSD |
| PCB | 1 | [ENP-00137](http://inventree.thielemann.intern/web/part/134/details) | SD-Card PCB Rev. 0.1, unpopulated, 38 x 64 mm |
| U2 | 1 | ENP-00102 | 24LC32 / 24LC32A, SOIC-8 |
| C1 | 1 | ENP-00006 | 100 nF X7R, 1206 |
| R1 | 1 | ENP-00018 | 10 kOhm, 1206 |
| J1 | 1 | ENP-00059 | 2x7, 2.54 mm, vertical bottom-mount header |
| U1 header | included in U1 | ENP-00019 only as replacement | 1x8, 2.54 mm, straight header; not a separate BOM line |

Mount U1 directly through the 1x8 header; no additional socket is planned.
Adafruit specifies the unattached header as included with product 254. The BOM
therefore counts it within ENP-00136, not again as ENP-00019. Check delivery
contents before assembly. Mechanical fastener/spacer parts and lengths remain
to be selected; ordered spacers are intentionally excluded from stock and BOM
until the user has checked them. The microSD card is separate operating
accessory/test equipment; its exact type remains to be documented.

ENP-00136 and ENP-00137 were newly created. The Reichelt supplier part for
ENP-00136 is saved; the separate Adafruit manufacturer-part link (MPN 254) is
still open. Existing standard-part records were reused.

### Kit and BOM

[ENK-00022 - EnvNode SD-Card Bausatz Rev. 0.1](http://inventree.thielemann.intern/web/part/135/bom)
is a loose-parts kit in category Kits, configured like the other module kits
(assembly/component, not purchased as a complete kit). Its BOM was created,
checked and marked validated in InvenTree on 20 October 2026:

| Ref. | Part | Qty |
| --- | --- | ---: |
| PCB | ENP-00137 | 1 |
| U1 | ENP-00136 | 1 |
| U2 | ENP-00102 | 1 |
| C1 | ENP-00006 | 1 |
| R1 | ENP-00018 | 1 |
| J1 | ENP-00059 | 1 |

Six required positions, no variants or optional lines. The header is included
with U1. Distanzhülsen/spacers and other mounting hardware are deliberately not
included; no stock receipts or consumption were posted for them. The microSD
card is separate accessory/test equipment. BOM validation records the checked
parts list, not completion of a physical build or field qualification.

## Verification - 20 October 2026

KiCad 10.0.5: DRC with zone refill and schematic parity check returned 0
violations, 0 unconnected pads, 0 footprint errors and 0 parity issues. ERC:
0 errors, 6 warnings (three unused interface labels and three mixed local/global
labels). See `Documentation/SD_Card_ERC_Rev0.1.txt` and
`Documentation/SD_Card_DRC_Rev0.1.txt` for the recorded check settings/results.
This is a CAD review, not a completed physical bring-up or field validation.

## Open work

- [x] Create U1 and PCB parts in **InvenTree** and identify the existing standard parts.
- [x] Create and validate **ENK-00022** with its six-position InvenTree BOM.
- [ ] Check ordered spacers before selecting their lengths/parts and adding them
  to stock or BOM; keep them excluded until approved.
- [ ] Link Adafruit as the separate manufacturer record for U1.
- [ ] Assemble and complete the measurement/bring-up record.
- [ ] Define and support a dedicated SD-Card module descriptor/profile; do not
  provision a descriptor for an unrelated module.
- [ ] Implement/test optional asynchronous diagnostic recording, export and
  graceful degradation on missing/full/failed SD cards before field use.
- [ ] Qualify the selected card, power budget, timestamp source and enclosure
  under the actual outdoor installation conditions.

## Original documentation

- https://www.adafruit.com/product/254
- https://learn.adafruit.com/adafruit-micro-sd-breakout-board-card-tutorial/download
- https://learn.adafruit.com/adafruit-micro-sd-breakout-board-card-tutorial/arduino-wiring
