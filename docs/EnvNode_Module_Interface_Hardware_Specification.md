# EnvNode Module Interface – Hardware Specification

**Status:** Draft  
**Revision:** 0.4

## 1. Purpose and Scope

The EnvNode Module Interface defines a generic internal electrical and mechanical interface for EnvNode expansion modules. It is an EnvNode-wide interface and is not specific to RainControl, cistern control, or any other individual application.

Potential modules include:

- analog input modules, for example for 4–20 mA sensors;
- relay and actuator modules;
- power modules, for example a 5 V to 24 V converter; and
- future I²C- or GPIO-based modules.

This revision records the agreed interface principles. Items that still require electrical or mechanical evaluation are explicitly listed as open points and are not specified prematurely.

## 2. Interface Architecture

An expansion module connects through two mechanically separated connectors designated **C1** and **C2**.

The two connectors used on a given board shall:

- use a keyed 2 × 4 contact arrangement on a 2.54 mm pitch;
- be mechanically identical;
- have the same orientation;
- use exactly the same standardized physical pin assignment.

On a mainboard, C1 and C2 shall be fully connected in parallel. A module shall preserve the standardized physical pin assignment on both connectors, but may leave unused interface pins electrically unconnected. Interface pins shall not be repurposed.

A module may connect a required power or signal net through either connector or through both connectors. Power and ground should be connected through both connectors where this is beneficial for current capacity or connection integrity.

The second connector is intended to provide additional mechanical support, particularly for larger modules or modules subject to wiring forces, such as relay boards with terminal blocks.

The exact connector part or connector family has not yet been selected. A keyed shrouded header and matching receptacle or cable connector is one possible implementation.

## 3. Electrical Interface

The interface assigns the following eight nets to the physical connector pins:

| Pin | Net | Function |
| ---: | --- | --- |
| 1 | `GND` | Common ground reference |
| 2 | `24V` | 24 V power rail |
| 3 | `3V3` | 3.3 V power rail |
| 4 | `5V` | 5 V power rail |
| 5 | `SDA` | I²C serial data |
| 6 | `SCL` | I²C serial clock |
| 7 | `GPIO1` | General-purpose signal |
| 8 | `GPIO2` | General-purpose signal |

The same pin assignment applies to C1 and C2 and to both sides of the interface. The connector positions do not provide independent power or signal channels.

In the common plan view of an assembled mainboard and module, pin 1 is located at the upper-right corner of each 2 × 4 connector. The mating mainboard header and bottom-mounted module receptacle shall place corresponding pin numbers at identical assembled XY positions.

### 3.1 System I²C Bus

On all ESP32-based EnvNode mainboard variants, the mandatory system I²C bus shall use the following GPIOs:

| Signal | ESP32 GPIO |
| --- | --- |
| `SDA` | GPIO21 |
| `SCL` | GPIO22 |

GPIO21 and GPIO22 are reserved for the system I²C bus and form part of the EnvNode hardware ABI. Variant-specific hardware, including future 868 MHz radio implementations, shall not use these GPIOs for other functions.

Mainboard-internal infrastructure devices, including the board identification EEPROM, shall use this system I²C bus.

The Full Mainboard additionally assigns GPIO25 (`SDA`) and GPIO26 (`SCL`) to its second I²C bus. These GPIOs are intended to remain available for the same purpose on a future 868 MHz mainboard variant.

### 3.2 Mainboard Identification

Starting with Mainboard Revision 0.2, EnvNode mainboards shall provide a non-volatile board identification device on the system I²C bus.

The board identification device is a Microchip `24AA025E48` EEPROM with a factory-programmed EUI-48 identifier. It shall be connected as follows:

| Property | Assignment |
| --- | --- |
| I²C bus | System I²C bus |
| `SDA` | GPIO21 |
| `SCL` | GPIO22 |
| I²C address | `0x50` |
| `A0` | GND |
| `A1` | GND |
| Supply | 3.3 V |

The EEPROM stores an EnvNode board identifier that maps to the same board definitions used by the EnvNode runtime firmware and the board provisioning firmware. The factory-programmed EUI-48 provides a unique physical identifier for each mainboard.

Mainboard Revision 0.1 does not contain a board identification EEPROM and shall be treated as a legacy board by the firmware.

These mainboard-specific requirements are included in this document while only one EnvNode mainboard family exists. They shall be moved into a dedicated mainboard hardware specification when another mainboard is derived.

## 4. Power-Rail Ownership

The `24V`, `5V`, and `3V3` nets are shared power rails that form part of the module interface. Depending on the EnvNode mainboard and installed modules, a rail may be supplied by either a mainboard or a module, and other participants may consume power from it.

For each power rail, **exactly one active source is permitted in a complete assembled system**. Multiple active sources on the same rail are not permitted unless a future revision explicitly defines a safe power-sharing or isolation mechanism.

System integration shall therefore establish rail ownership before a mainboard and its modules are combined.

Example configurations include:

- A standard 5 V-powered EnvNode may provide `5V` and `3V3`. An optional power module may generate and source `24V` from `5V` when a 24 V rail is required.
- A Control/Cistern mainboard may be powered from 24 V, source `24V` onto the interface, and generate the `5V` and `3V3` rails locally.

These examples illustrate possible rail ownership; they do not define mandatory power architectures for all EnvNode products.

## 5. Mechanical Scope

The EnvNode Module Interface standardizes the connector-based mechanical relationship between a mainboard and a module. It does not standardize the overall module PCB size, outline, or shape.

The pin-1 coordinates of C1 and C2 shall have a horizontal center-to-center spacing of **25.40 mm** with no vertical offset. Both connectors shall have the same orientation. Connector spacing is therefore defined as:

```text
C1 pin 1 to C2 pin 1:
X = 25.40 mm
Y = 0.00 mm
```

The overall module PCB dimensions, outline, mounting-hole pattern, and connector-to-board-edge positions are not standardized by the electrical interface. Reusable HalfSize and FullSize reference templates may define project-specific module outlines without changing the connector interface itself.

The module-side receptacles are mounted on the bottom side of the module PCB. Their footprints shall account for the mirrored bottom-side geometry while retaining the standardized assembled pin positions. The EnvNode bottom-mounted receptacle footprint uses pre-mirrored pad locations for this purpose.

The initial mechanical exploration will include a representative two-relay module with flyback diodes and associated components. Its purpose is to determine practical connector geometry and packaging constraints; its dimensions are not defined by this revision.

### 5.1 Daughterboard Reference Design Block

The KiCad reference implementation for the module side is:

`EnvNodeDesignBlocks:EnvNode_Module_Interface_Daughterboard`

Its editable source is located in `hardware/kicad/DesignBlocks/EnvNode_Module_Interface_Daughterboard/`; the published copy is stored in the project design-block library below `hardware/kicad/Libraries/EnvNodeDesignBlocks.kicad_blocks/`.

The reference block provides:

- two bottom-mounted `PinSocket_2x04_P2.54mm_Vertical_Bottom` footprints;
- identical J1 and J2 pin assignments;
- 25.40 mm horizontal connector spacing with no vertical offset;
- parallel routing of `24V`, `3V3`, and `5V` using 0.40 mm tracks;
- parallel routing of `SDA`, `SCL`, `GPIO1`, and `GPIO2` using 0.20 mm tracks; and
- GND connectivity intended to be completed by a copper zone on the module PCB.

These track widths are a reference-layout baseline, not an interface current rating. Each completed module must still be checked for its actual current, copper thickness, temperature rise, protection, and connector limits.

Detailed usage and maintenance instructions are provided in `hardware/kicad/DesignBlocks/EnvNode_Module_Interface_Daughterboard/README.md`.

### 5.2 FullSize Reference Template

`hardware/kicad/Modules/EmptyModule_FullSize/` is the validated project starting point for full-size EnvNode daughterboards. It applies the daughterboard reference design block to a 38.00 mm x 64.00 mm PCB outline and completes GND with a copper zone.

The FullSize dimensions are a project reference geometry and do not change the electrical interface requirements. Detailed geometry, derivation, validation, and design-block update instructions are provided in `hardware/kicad/Modules/EmptyModule_FullSize/README.md`.

The HalfSize template remains provisional until it has independently passed the same checks.

## 6. Open Points for Future Revisions

The following items remain intentionally open:

1. Exact connector family and part numbers.
2. Mated connector stack height and permitted tolerance.
3. Current limits for each power rail, each connector contact, and the interface as a whole.
4. GPIO voltage levels, direction rules, drive capability, default states, and permitted alternate functions.
5. Required protection, including reverse-current, overvoltage, overcurrent, transient, and ESD protection.
6. Required behavior when one or more power rails are absent or unpowered.
7. I²C pull-up ownership, permitted pull-up values, bus voltage, bus capacitance, speed, and wiring or topology requirements beyond the fixed system-bus GPIO assignment and board-EEPROM address defined above.
8. PCB-edge placement, allowable board overhang, component keep-out areas, mating clearance, and enclosure constraints.
9. Any required module identification, capability declaration, or rail-source indication mechanism.

## 7. Revision Status

Revision 0.4 is a design draft. It establishes the shared interface concept, connector duplication, physical pin assignment, 25.40 mm connector spacing, mainboard-versus-module connectivity rules, the single-source-per-rail rule, the mandatory system I²C GPIO assignment, the Mainboard Revision 0.2 identification EEPROM, the validated daughterboard reference design block, and the validated FullSize project template. It is not yet sufficient for interchangeability without project-specific agreement on the remaining open electrical and mechanical points above.
