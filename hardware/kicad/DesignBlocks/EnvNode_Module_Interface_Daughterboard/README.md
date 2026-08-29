# Legacy EnvNode Daughterboard Module Interface Design Block

> **Obsolete Revision 0.1 reference:** This directory describes the former two-connector 2 x 4 interface, including a 24 V rail. It must not be used for new Revision 0.2 modules.
>
> Current module projects must use `EnvNode_Module_Connector_Daughterboard_SMD` or `EnvNode_Module_Connector_Daughterboard_THT` and the 2 x 7 pin assignment in `docs/EnvNode_Module_Interface_Hardware_Specification.md`.

## Purpose

`EnvNode_Module_Interface_Daughterboard` is the reference KiCad design block for the module side of the EnvNode expansion interface. It provides two electrically parallel 2 x 4 connectors for daughterboards that plug onto an EnvNode mainboard.

The design block defines the electrical pin assignment, connector geometry, bottom-mounted connector footprints, and the routed interconnect between both connectors. Module-specific circuitry is added outside this block.

## Source and Published Library Block

- Editable source project: `hardware/kicad/DesignBlocks/EnvNode_Module_Interface_Daughterboard/`
- Published design block: `hardware/kicad/Libraries/EnvNodeDesignBlocks.kicad_blocks/EnvNode_Module_Interface_Daughterboard.kicad_block/`
- Library identifier: `EnvNodeDesignBlocks:EnvNode_Module_Interface_Daughterboard`

The schematic and PCB portions are updated separately in KiCad. Changes to the source schematic must be saved to the linked design block from the Schematic Editor. Layout changes must be saved separately from the PCB Editor.

## Connector Assignment

J1 and J2 use the same pin assignment and are connected in parallel:

| Pin | Net | Function |
| ---: | --- | --- |
| 1 | `GND` | Common ground reference |
| 2 | `24V` | 24 V power rail |
| 3 | `3V3` | 3.3 V power rail |
| 4 | `5V` | 5 V power rail |
| 5 | `SDA` | System I2C data |
| 6 | `SCL` | System I2C clock |
| 7 | `GPIO1` | General-purpose signal |
| 8 | `GPIO2` | General-purpose signal |

Unused pins may remain unconnected in module-specific circuitry, but their assigned function must not be changed.

## Mechanical Definition

Both connectors use the custom footprint:

`EnvNode-Footprints:PinSocket_2x04_P2.54mm_Vertical_Bottom`

This footprint is intended for receptacles mounted on the bottom side of the daughterboard and preserves the standardized pin numbering in the assembled stack.

The connector reference positions in the published block are:

| Connector | X | Y | Rotation |
| --- | ---: | ---: | ---: |
| J1 | 150.57 mm | 62.17 mm | 180 degrees |
| J2 | 125.17 mm | 62.17 mm | 180 degrees |

The absolute coordinates are an implementation detail. The normative relationship is:

- horizontal spacing: 25.40 mm;
- vertical offset: 0.00 mm; and
- identical orientation.

Moving or rotating the complete design block is permitted. Changing the relative connector geometry is not.

## Routing Baseline

The published PCB block uses the following track widths:

| Nets | Track width |
| --- | ---: |
| `24V`, `5V`, `3V3` | 0.40 mm |
| `SDA`, `SCL`, `GPIO1`, `GPIO2` | 0.20 mm |

`GND` is intended to be connected by a copper zone on the completed module PCB. The track widths describe the reusable connector interconnect and are not a declaration of the permitted interface current. Current limits remain dependent on connector ratings, copper thickness, thermal conditions, and the completed module design.

## Use in a Module

1. Place the schematic portion of the design block in the module schematic.
2. Update the PCB from the schematic with symbol grouping enabled where appropriate.
3. Apply or place the linked PCB layout once.
4. Add the module outline, GND zone, mounting features, and module-specific circuitry.
5. Do not place a second overlapping copy of the PCB block. Exact same-net overlaps may not be reported by DRC.
6. Run ERC and DRC on the completed module project.

## Validated Library State

The published library block was checked on 2026-08-26 with KiCad 10:

- both schematic symbols use the Bottom footprint assignment;
- J1 and J2 have identical pin-to-net mappings;
- connector spacing is 25.40 mm with no vertical offset;
- all 30 routed segments are unique;
- power and signal routing uses the widths listed above; and
- schematic ERC reports no errors or warnings.

Validation of a complete module remains mandatory because the library PCB fragment does not include the module outline, module-specific circuitry, or the final copper zones.
