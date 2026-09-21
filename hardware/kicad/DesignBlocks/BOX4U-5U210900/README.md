# BOX4U 5U210900 PCB template

KiCad board template for mounting a PCB in the base of the BOX4U 5U210900 enclosure.

The design block contains both the schematic representation and the PCB geometry. `H1` through `H4` are non-electrical mounting-hole symbols, excluded from BOM and position files, and linked to the matching project footprint.

## Geometry

- Board outline: 130 mm x 68 mm, corner radius 3 mm
- Mounting-hole pattern: 121.5 mm x 58 mm, centered on the board
- Mounting holes: four NPTH holes, diameter 4.5 mm
- Intended fastener: 4 mm self-tapping screw into the plastic bosses (not M4)
- Keepout/courtyard: diameter 9 mm around every mounting hole
- Origin/reference center: board center at X=100 mm, Y=100 mm

The solid `Edge.Cuts` geometry defines the PCB. Dashed `Dwgs.User` geometry documents the enclosure's inner wall and unused outer screw bosses from the manufacturer drawing; it is not part of the routed PCB outline.

Source drawing: `hardware/housings/5U210900.pdf` (BOX4U drawing 5U210900, PC 210900 5U enclosure).

Before ordering PCBs, verify the outline, hole pattern, screw-head clearance, and assembly fit against a physical enclosure.
