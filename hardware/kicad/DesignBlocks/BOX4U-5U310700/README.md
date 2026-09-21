# BOX4U 5U310700 PCB template

KiCad board template for mounting a PCB in the base of the BOX4U 5U310700 enclosure.

The design block contains both the schematic representation and the PCB geometry. `H1` through `H4` are non-electrical mounting-hole symbols, excluded from BOM and position files, and linked to the matching project footprint.

## Geometry

- Board outline: 113 mm x 93 mm, corner radius 3 mm
- Mounting-hole pattern: 98 mm x 83.4 mm, centered on the board
- Mounting holes: four NPTH holes, diameter 4.5 mm
- Intended fastener: self-tapping screw into the enclosure's plastic bosses, not a metric through-bolt
- Keepout/courtyard: diameter 9 mm around every mounting hole
- Origin/reference center: board center at X=100 mm, Y=100 mm

The solid `Edge.Cuts` geometry defines the PCB. Dashed `Dwgs.User` geometry documents the enclosure reference outline and lid screw bosses from the manufacturer drawing; it is not part of the routed PCB outline.

Source drawing: `hardware/housings/5U310700.pdf` (BOX4U drawing 5U310700, ABS 310700 5U enclosure).

The 113 mm x 93 mm outline and the 98 mm x 83.4 mm mounting pattern agree with the initial EnvNode Weatherstation Rev. 0.1 layout. Before ordering PCBs, verify the outline, hole pattern, screw-head clearance, connector access, and assembly fit against a physical enclosure.
