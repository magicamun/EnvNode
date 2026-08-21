# EnvNode Module Interface – Hardware Specification

**Status:** Draft  
**Revision:** 0.1

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

Both connectors shall:

- use a keyed 2 × 4 contact arrangement on a 2.54 mm pitch;
- be mechanically identical;
- have the same orientation;
- have exactly the same electrical pinout; and
- be electrically connected in parallel.

The second connector is intended to provide additional mechanical support, particularly for larger modules or modules subject to wiring forces, such as relay boards with terminal blocks.

The exact connector part or connector family has not yet been selected. A keyed shrouded header and matching receptacle or cable connector is one possible implementation.

## 3. Electrical Interface

The interface provides the following eight distinct nets on each connector:

| Net | Function |
| --- | --- |
| `24V` | 24 V power rail |
| `5V` | 5 V power rail |
| `3V3` | 3.3 V power rail |
| `GND` | Common ground reference |
| `SDA` | I²C serial data |
| `SCL` | I²C serial clock |
| `GPIO1` | General-purpose signal |
| `GPIO2` | General-purpose signal |

Each net appears once on C1 and at the corresponding position on C2. Because C1 and C2 are electrically parallel, they do not provide independent power or signal channels.

The mapping of these nets to physical connector pin numbers remains open.

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

Connector placement, spacing, orientation, mating geometry, and applicable clearance or overhang rules will be defined after mechanical evaluation. Until then, no connector-to-connector distance, PCB dimension, mounting-hole pattern, or board-edge position is normative.

The initial mechanical exploration will include a representative two-relay module with flyback diodes and associated components. Its purpose is to determine practical connector geometry and packaging constraints; its dimensions are not defined by this revision.

## 6. Open Points for Future Revisions

The following items remain intentionally open:

1. Exact connector family and part numbers.
2. Center-to-center spacing and relative placement of C1 and C2.
3. Physical pin-number mapping for `24V`, `5V`, `3V3`, `GND`, `SDA`, `SCL`, `GPIO1`, and `GPIO2`.
4. Current limits for each power rail, each connector contact, and the interface as a whole.
5. GPIO voltage levels, direction rules, drive capability, default states, and permitted alternate functions.
6. Required protection, including reverse-current, overvoltage, overcurrent, transient, and ESD protection.
7. Required behavior when one or more power rails are absent or unpowered.
8. I²C pull-up ownership, permitted pull-up values, bus voltage, bus capacitance, speed, addressing, and wiring or topology requirements.
9. PCB-edge placement, allowable board overhang, component keep-out areas, mating clearance, and enclosure constraints.
10. Any required module identification, capability declaration, or rail-source indication mechanism.

## 7. Revision Status

Revision 0.1 is a design draft. It establishes the shared interface concept, connector duplication, net set, and single-source-per-rail rule. It is not yet sufficient for interchangeability without project-specific agreement on the open electrical and mechanical points above.

