# ORing Power S3 — Revision 0.1

Source-available hardware; see the repository `LICENSE`.

Compact **39 × 8.5 mm** castellated power daughterboard. This is a separate
project from the 39 × 15 mm ORing Power board: it has **no USB-C connector,
JST-PH connector or USB-C CC resistors**. The carrier supplies 5 V through U2.

## Electrical interface

| U2 pad | Net | Function |
| ---: | --- | --- |
| 1 | GND | Common ground |
| 2 | +5V | Regulator input from the carrier |
| 3 | +3V3_EXT | LM3940 output, before D1 |
| 4 | +3V3_DEVKIT | DevKit 3.3 V input, before D2 |
| 5 | +3V3_SYS | Shared system supply, after D1/D2 |

D1/D2 are SS14 Schottky diodes. C2 is the 47 µF system reservoir;
C4 is the 47 µF tantalum capacitor directly at the regulator output.
C3 is the 10 µF input capacitor; C1/C5 are 100 nF bypass capacitors.
The system voltage includes the forward drop of the selected ORing diode.

The schematic symbol is `EnvNode-Symbols:ORingPower_S3_Module`.
The PCB uses `EnvNode-Footprints:ORingPower_S3_Module_Castellated`.
Its five numbered signals retain the ORing interface names, but the compact
module outline and pad positions must be checked against a matching S3 carrier.
The older module's carrier footprint must not be assumed mechanically compatible.

## Bring-up

Before fitting a mainboard/DevKit, check continuity and supply-to-ground isolation,
capacitor polarity, the 5 V input, regulator output and system supply. Then test
source handover, reverse feeding, load steps and temperature under the intended
load. The layout checks do not replace these physical tests.

Documentation copied from the 39 × 15 mm ORing project does not describe this
board. Use the project-specific validation report and this interface table.

## Validation

Checked on 2026-11-09 with KiCad 10.0.5: ERC 0, DRC 0, unconnected items 0,
schematic/PCB mismatches 0. See [validation record](Documentation/Validation-0.1.json),
including deliberately disabled DRC rules for the castellated geometry.
Physical qualification remains open.
