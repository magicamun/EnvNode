# EnvNode RainDetector Controller — Revision 0.3

ICM7555 frequency measurement front end at 3.3 V and AO3400A low-side heater
switch. Timing resistors are 10 kohms and 100 kohms; C1 is the 100 nF supply
bypass. The 5 V heater path uses 0.8 mm tracks and must supply approximately
417 mA for RainDetectorHead revision 0.1, in addition to other system loads.

J1/J2 sockets are on B.Cu, facing the head; controller components are on F.Cu
for access from the enclosure interior. The board is 54.12 x 44.12 mm.
See `../RainDetectorHead/README.md` for the complete mating table.

J3: 1 GND, 2 +5V, 3 HEATER_EN, 4 +3V3.
J4: 1 GND, 2 +5V, 3 RAIN_FREQ, 4 +3V3.
The supply pins are shared. J3/J4 are currently marked DNP in the schematic
and generated BOM; retain or change this deliberately for the intended
connector/assembly option. Concrete connector part numbers and mating height
still need selection and verification.

`RainDetectorController.csv` is generated from the current schematic:

```sh
kicad-cli sch export bom --group-by Value,Footprint -o RainDetectorController.csv RainDetectorController.kicad_sch
```

Historical reference/bring-up PDFs remain in `../Raindetector/Documentation/`.
They are NOT revision 0.3 assembly guides. The old `../Raindetector/` project
remains as the previous prototype; local copied PDFs need not be duplicated
in version control.

## Validation

For each of the two projects, run from its directory:

```sh
kicad-cli sch erc -o /tmp/rain-erc.txt PROJECT.kicad_sch
kicad-cli pcb drc --schematic-parity --refill-zones -o /tmp/rain-drc.txt PROJECT.kicad_pcb
```

Replace PROJECT with RainDetectorController or RainDetectorHead. Checked with
KiCad 10.0.5. Standard project-disabled checks (such as missing courtyards)
remain as configured; these commands do not constitute fabrication approval.
The lab firmware in `lab/RainDetector-PCNT/` measures rising edges via ESP32
PCNT and reports raw frequency over Serial. No heater automation is included.

Final checks on 2026-09-14: both projects passed ERC and DRC with zero
errors/warnings, zero unconnected items and zero schematic parity issues.
The Head GND power flag and C1-only 0.10 mm clearance rule are included.
