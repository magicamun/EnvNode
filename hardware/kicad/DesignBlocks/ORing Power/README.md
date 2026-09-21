# ORing Power Module

The ORing Power module is a 39 mm x 15 mm castellated daughterboard for EnvNode mainboards. It combines the external 5 V input, the 3.3 V regulator, and the diode ORing between the regulator output and the ESP32 DevKit 3.3 V rail. Mainboards use the matching carrier footprint and no longer need to reproduce this circuit locally.

## Project and Library Items

- Editable KiCad project: `hardware/kicad/DesignBlocks/ORing Power/`
- Published design block: `hardware/kicad/Libraries/EnvNodeDesignBlocks.kicad_blocks/ORing Power.kicad_block/`
- Mainboard symbol: `EnvNode-Symbols:ORingPower_Module`
- Module footprint: `EnvNode-Footprints:ORingPower_Module_Castellated`
- Mainboard carrier footprint: `EnvNode-Footprints:ORingPower_Carrier`

The symbol defaults to the carrier footprint. The ORing Power PCB itself overrides that assignment with the module-side castellated footprint.

## Electrical Function

The module accepts 5 V through either the two-pin JST-PH connector or the USB-C connector. These two 5 V inputs share the same `+5V` net and are not isolated from one another. Do not connect independent supplies to both connectors unless they are explicitly suitable for direct parallel operation.

The LM3940 generates `+3V3_EXT` from `+5V`. Two SS14 Schottky diodes combine `+3V3_EXT` and the mainboard-provided `+3V3_DEVKIT` rail into `+3V3_SYS`. The topology prevents either 3.3 V source from feeding back into the other through the ORing path.

The USB-C connector is the six-contact, power-only GCT `USB4125-GF-A-0190`. `CC1` and `CC2` each have their own 5.1 kΩ pull-down resistor to GND. The connector does not expose USB data signals and does not implement USB Power Delivery negotiation; it requests the standard USB-C 5 V source mode only.

## Castellated Interface

| Pin | Net | Module direction | Function |
| ---: | --- | --- | --- |
| 1 | `GND` | Common | Ground |
| 2 | `+5V` | Output to mainboard | Shared 5 V input rail |
| 3 | `+3V3_EXT` | Output to mainboard | LM3940 regulator output before ORing diode |
| 4 | `+3V3_DEVKIT` | Input from mainboard | ESP32 DevKit 3.3 V source before ORing diode |
| 5 | `+3V3_SYS` | Output to mainboard | ORed system 3.3 V rail |

Pin numbers, net names, pad positions, and module orientation are contractual. A carrier PCB must use the matching carrier footprint without moving individual pads.

## Mechanical Integration

The module is soldered directly onto the mainboard using the five castellated edge pads. The carrier footprint reserves the 39 mm x 15 mm module outline and provides matching mainboard pads. Before releasing a mainboard, verify:

- the module outline and nearby component courtyards do not overlap;
- the USB-C opening is reachable at the intended enclosure edge;
- sufficient solder fillet is accessible at every castellated pad;
- no mainboard copper, vias, or components occupy the reserved module area; and
- the assembly process supports castellated-hole soldering on the selected PCB finish and thickness.

The castellated-hole geometry must be confirmed with the PCB manufacturer before the first production order. Fabrication outputs must identify the edge-plated half holes correctly; ordinary routed holes without edge plating are not an equivalent substitute.

## Component Notes

| Reference | Function | Important assignment |
| --- | --- | --- |
| J3 | USB-C 5 V input | GCT `USB4125-GF-A-0190`; KiCad six-pin power-only footprint |
| R1, R2 | USB-C Rd | 5.1 kΩ each, one resistor per CC pin |
| J1 | Alternate 5 V input | JST `B2B-PH-SM4-TB`, two-pin, vertical SMD; KiCad footprint `JST_PH_B2B-PH-SM4-TB_1x02-1MP_P2.00mm_Vertical` |
| U1 | 3.3 V regulator | `LM3940IMP-3.3/NOPB`, SOT-223 |
| D1, D2 | 3.3 V source ORing | SS14, SMA |
| C1, C5 | Local bypass | 100 nF, 1206 |
| C2 | Regulator output capacitor | 10 µF electrolytic, `CP_Elec_5x5.4`; polarity required |
| C3 | 5 V input capacitor | 10 µF ceramic, 1206 |
| C4 | System rail bulk capacitor | 47 µF tantalum, EIA-3528-21; polarity required |

## Mainboard Use

1. Place `EnvNode-Symbols:ORingPower_Module` in the mainboard schematic.
2. Keep its default `EnvNode-Footprints:ORingPower_Carrier` footprint.
3. Connect all five named rails according to the interface table.
4. Update the PCB from the schematic and place the complete carrier footprint at the intended board edge.
5. Remove the former discrete ORing-power circuit and verify that no duplicate regulator, diode, USB-C, or input components remain.
6. Run ERC, PCB/schematic parity, DRC, and a visual 3D/mechanical review on the completed mainboard.

Do not place the module-side castellated footprint on the mainboard. Do not use the carrier footprint as the outline of a separately fabricated ORing Power PCB.

## Validation Status

Checked with KiCad 10 on 2026-09-21:

- schematic ERC: 0 errors, 0 warnings;
- PCB DRC: 0 violations and 0 unconnected items;
- all seven electrical nets have zero unrouted connections;
- USB VBUS, GND, shield, CC1, and CC2 map to the expected footprint pads;
- the GCT USB4125 footprint is aligned with the PCB edge; and
- the editable source and published library block are synchronized.

This validation covers design consistency, not physical manufacture. The first fabricated module still requires continuity, polarity, 5 V input, 3.3 V regulation, source handover, reverse-current, thermal, USB-C insertion, and castellated-solder-joint tests.
