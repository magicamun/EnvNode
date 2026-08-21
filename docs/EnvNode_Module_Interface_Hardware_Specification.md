# EnvNode Module Interface – Hardware Specification

**Status:** Draft  
**Revision:** 0.2

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

The overall module PCB dimensions, outline, mounting-hole pattern, and connector-to-board-edge positions are not standardized.

The module-side receptacles are mounted on the bottom side of the module PCB. Their footprints shall account for the mirrored bottom-side geometry while retaining the standardized assembled pin positions. The EnvNode bottom-mounted receptacle footprint uses pre-mirrored pad locations for this purpose.

The initial mechanical exploration will include a representative two-relay module with flyback diodes and associated components. Its purpose is to determine practical connector geometry and packaging constraints; its dimensions are not defined by this revision.

## 6. Open Points for Future Revisions

The following items remain intentionally open:

1. Exact connector family and part numbers.
2. Mated connector stack height and permitted tolerance.
3. Current limits for each power rail, each connector contact, and the interface as a whole.
4. GPIO voltage levels, direction rules, drive capability, default states, and permitted alternate functions.
5. Required protection, including reverse-current, overvoltage, overcurrent, transient, and ESD protection.
6. Required behavior when one or more power rails are absent or unpowered.
7. I²C pull-up ownership, permitted pull-up values, bus voltage, bus capacitance, speed, addressing, and wiring or topology requirements.
8. PCB-edge placement, allowable board overhang, component keep-out areas, mating clearance, and enclosure constraints.
9. Any required module identification, capability declaration, or rail-source indication mechanism.

## 7. Revision Status

Revision 0.2 is a design draft. It establishes the shared interface concept, connector duplication, physical pin assignment, 25.40 mm connector spacing, mainboard-versus-module connectivity rules, and the single-source-per-rail rule. It is not yet sufficient for interchangeability without project-specific agreement on the remaining open electrical and mechanical points above.
