# EnvNode EmptyModule HalfSize

## Purpose

`EmptyModule_HalfSize` is the KiCad starting point for compact EnvNode daughterboards. It combines the standardized module-side connector interface with a 38 mm x 32 mm module outline and a GND copper zone.

The template is derived from the validated FullSize project. The schematic, connector interface, linked design block, footprint placement, and routing are unchanged; only the board outline, GND-zone extent, project identity, and board labeling are adapted to HalfSize.

Copy this project when creating a compact module. Add module-specific circuitry without changing the standardized connector assignment or the relative connector geometry.

## Project Files

- `EmptyModule_HalfSize.kicad_pro`
- `EmptyModule_HalfSize.kicad_sch`
- `EmptyModule_HalfSize.kicad_pcb`

The connector interface is linked to:

`EnvNodeDesignBlocks:EnvNode_Module_Interface_Daughterboard`

The design block is documented in `hardware/kicad/DesignBlocks/EnvNode_Module_Interface_Daughterboard/README.md`.

## Mechanical Definition

| Property | Value |
| --- | ---: |
| Board width | 38.00 mm |
| Board height | 32.00 mm |
| J1 position | X 156.49 mm, Y 75.26 mm, 180 degrees |
| J2 position | X 131.09 mm, Y 75.26 mm, 180 degrees |
| J1-to-J2 horizontal spacing | 25.40 mm |
| J1-to-J2 vertical offset | 0.00 mm |

The absolute coordinates apply to this template. The interface requirement is the relative 25.40 mm horizontal spacing, zero vertical offset, and identical connector orientation.

Both receptacles are mounted on the bottom side using:

`EnvNode-Footprints:PinSocket_2x04_P2.54mm_Vertical_Bottom`

The HalfSize outline is a project reference geometry, not a requirement of the electrical EnvNode Module Interface. A derived board may only be described as HalfSize-compatible if it preserves this outline and connector placement or explicitly documents its deviation.

## Electrical Interface

J1 and J2 are connected in parallel:

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

The reusable interconnect uses 0.40 mm tracks for `24V`, `3V3`, and `5V`, and 0.20 mm tracks for `SDA`, `SCL`, `GPIO1`, and `GPIO2`. GND is completed by the module copper zone.

These widths are a routing baseline and do not specify an allowed interface current. The completed module must be evaluated for its actual current, copper thickness, temperature rise, connector limits, and protection requirements.

## Creating a Derived Module

1. Copy the complete `EmptyModule_HalfSize` project directory and rename the project files consistently.
2. Preserve the linked design block group in both schematic and PCB.
3. Add module-specific circuitry and components within the reduced HalfSize area.
4. Confirm rail ownership before connecting a circuit that can source `24V`, `5V`, or `3V3`.
5. Refill copper zones and run ERC and DRC before release.
6. Check assembled height, mating connector, mainboard clearance, and application-specific isolation requirements.

## Updating the Linked Design Block

KiCad stores the schematic and PCB portions of a design block separately. Use the following workflow when the library block changes:

1. In the schematic, preserve the J1/J2 annotations when replacing the linked block group.
2. Run **Update PCB from Schematic**.
3. Enable **Re-link footprints to schematic symbols based on their reference designators** so that the existing J1/J2 footprints are reused instead of inserted again.
4. Enable grouping based on schematic symbol groups where applicable.
5. In the PCB Editor, select the linked group and use **Apply Design Block Layout**.
6. Confirm that the result contains one linked group, two connector footprints, and 30 unique track segments.
7. Refill zones, save, and rerun ERC and DRC.

If only the stored PCB layout changed and the placed schematic block did not change, an additional schematic-to-PCB update is normally unnecessary. Apply the linked design block layout directly to the existing PCB group.

Exact same-net track overlaps may not be reported by DRC. Therefore, checking the group count and unique segment count is part of the update procedure.

## Checked State

The template was checked on 2026-08-26 with KiCad 10:

- schematic ERC: no errors or warnings;
- exactly one linked daughterboard-interface group;
- two correctly assigned Bottom connector footprints;
- 30 routed segments, all unique;
- correct power and signal track widths;
- correct 38 mm x 32 mm outline;
- correct 25.40 mm connector spacing with no vertical offset;
- one GND copper zone; and
- successful board parsing and 3D construction.

The command-line DRC currently aborts internally for this template without producing a violation report, as it does for the corresponding FullSize project after the design-block update. Run DRC in the KiCad PCB Editor after refilling the zones before release. Every derived module additionally requires its own final electrical, mechanical, and safety review.
