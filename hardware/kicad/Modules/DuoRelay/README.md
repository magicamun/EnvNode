# EnvNode DuoRelay FullSize

## Purpose

`DuoRelay` is a FullSize EnvNode daughterboard with two independently controlled electromechanical changeover relays. It is intended for switching two external loads from `GPIO1` and `GPIO2` while keeping the relay contact circuits separated from the EnvNode SELV control circuitry.

The module is derived from `EmptyModule_FullSize` and uses the standardized daughterboard interface design block.

## Project Files

- `DuoRelay.kicad_pro`
- `DuoRelay.kicad_sch`
- `DuoRelay.kicad_pcb`
- `DuoRelay.kicad_dru`

`DuoRelay.kicad_dru` contains the safety-related custom clearance rule and is a required part of the project. It must be committed and distributed with the other KiCad files.

## Module Interface

J1 and J2 are bottom-mounted module receptacles using:

`EnvNode-Footprints:PinSocket_2x04_P2.54mm_Vertical_Bottom`

| Pin | Net | DuoRelay use |
| ---: | --- | --- |
| 1 | `GND` | Relay-driver reference and emitter return |
| 2 | `24V` | Passed through between J1 and J2; unused by the relay circuit |
| 3 | `3V3` | Passed through between J1 and J2; unused by the relay circuit |
| 4 | `5V` | Supplies both relay coils |
| 5 | `SDA` | Passed through between J1 and J2 |
| 6 | `SCL` | Passed through between J1 and J2 |
| 7 | `GPIO1` | Controls relay K1 |
| 8 | `GPIO2` | Controls relay K2 |

The two 5 V relay coils draw approximately 38.5 mA each, or approximately 77 mA when both relays are energized. The supplying mainboard must include this load in its 5 V power budget.

## Relay and Driver Circuits

K1 and K2 use:

`Finder 34.51.7.005.0010`

Relevant relay properties include a 5 V sensitive DC coil, approximately 130 ohm coil resistance, 170 mW coil power, one changeover contact, and reinforced separation between coil and contacts according to the relay manufacturer.

Each relay is driven by a BC817 NPN low-side stage:

- K1: `GPIO1` through R1 to Q1;
- K2: `GPIO2` through R2 to Q2;
- R1/R2: 1 kohm base resistors;
- R3/R4: 100 kohm base-to-emitter pulldowns; and
- D1/D2: 1N4148W flyback diodes across the relay coils.

The pulldowns keep both transistors off while the GPIOs are high impedance, including during startup or reset. The daughterboard therefore owns the safe relay-off behavior; firmware must still configure both GPIOs as inactive outputs during initialization.

The flyback diode cathodes connect to `5V`; their anodes connect to the transistor collectors and relay coil A2 terminals.

No additional bulk capacitor is fitted by default. The local flyback diodes handle coil turn-off current, and the mainboard owns the shared 5 V supply stability. Supply behavior should be verified during system bring-up with both relays switching.

## Contact Terminals

The two terminal blocks are intentionally mirrored to permit via-free top-layer routing while preserving the required separation distances:

| Terminal | Pin 1 | Pin 2 | Pin 3 |
| --- | --- | --- | --- |
| J3 / K1 | `NO1` | `COM1` | `NC1` |
| J4 / K2 | `NC2` | `COM2` | `NO2` |

The PCB silkscreen labels each terminal position as `NO`, `COM`, or `NC`. The mirrored order is intentional and must be preserved or explicitly redesigned and revalidated.

## Current Limits

The relay itself is rated higher than the released module current. The DuoRelay PCB rating is limited by the present 1.5 mm external-layer copper routing and the absence of a completed application-specific load qualification.

For a board manufactured with at least 35 micrometers of external copper:

- maximum released load: **3 A RMS per channel at 230/250 VAC for resistive loads**;
- provisional limit for inductive loads: **2 A RMS per channel**, pending validation of the actual load, inrush current, switching category, contact life, and suppression; and
- higher currents are not released by this board revision even though the relay component has a 6 A contact rating.

The limits apply per channel. They are design limits, not a product safety certification. Loads with high inrush current, including motors, pumps, transformers, lamps, capacitive supplies, contactors, and solenoids, require separate evaluation. External fusing and load-side transient suppression must be defined by the system design.

## PCB Isolation and Routing

The contact nets `NO1`, `COM1`, `NC1`, `NO2`, `COM2`, and `NC2` belong to the `250V` net class.

The layout uses:

- 1.5 mm tracks for all contact nets;
- contact routing only on `F.Cu`;
- no vias in the contact routing;
- no GND copper fill in the load-side region;
- a GND zone restricted to the SELV control-side region; and
- an 8 mm custom clearance between the `250V` net class and all non-`250V` copper.

The custom rule in `DuoRelay.kicad_dru` is:

```scheme
(version 1)

(rule "250VAC to SELV clearance"
    (severity error)
    (condition "(A.hasNetclass('250V') && !B.hasNetclass('250V')) || (B.hasNetclass('250V') && !A.hasNetclass('250V'))")
    (constraint clearance (min 8mm))
)
```

The rule deliberately applies only between `250V` and non-`250V` copper. It does not require 8 mm between `NO`, `NC`, and `COM`, because those nets must be routed through the relay and 5.08 mm terminal geometries. The `250V` net class separately specifies 1.5 mm nominal track width and 1.5 mm same-class clearance.

Any change to the relay or terminal footprints, placement, copper-zone boundary, track geometry, net-class patterns, or custom rule requires a new isolation and DRC review.

## Mechanical and Safety Integration

Routing the contact tracks on `F.Cu` and removing the load-side GND fill prevents ordinary board copper from entering the contact region. It does not remove mains voltage from the bottom side: the relay and terminal-block contact pads are plated through-hole pads, and their solder joints are exposed on the underside.

Before the module is used with hazardous voltage, the complete assembly must verify:

- vertical air clearance from underside solder joints to the mainboard;
- absence of mainboard copper, components, fasteners, or conductive enclosure parts within the required clearance region;
- connector stack height and mechanical tolerances;
- protection against accidental access;
- enclosure, wiring, strain relief, fuse, earthing, and installation requirements; and
- applicable product and installation standards.

An insulating sheet or fixed barrier may be required by the final mechanical design. The PCB DRC checks planar copper geometry only and does not validate the assembled three-dimensional insulation system.

## Bring-up Checklist

1. Inspect relay orientation, diode polarity, transistor orientation, terminal orientation, and solder joints.
2. Confirm that J3 is labeled `NO-COM-NC` and J4 is labeled `NC-COM-NO` when viewed from the component side.
3. Power the module from a current-limited 5 V SELV supply without mains connected.
4. Confirm both relays remain released when `GPIO1` and `GPIO2` are floating or inactive.
5. Activate GPIO1 and verify only K1 switches.
6. Activate GPIO2 and verify only K2 switches.
7. Measure 5 V behavior while switching each relay and both relays together.
8. Verify NO/COM/NC continuity for both relay states before connecting a load.
9. Refill zones and run ERC and DRC with `DuoRelay.kicad_dru` loaded.
10. Perform the mechanical isolation review before applying hazardous voltage.

Initial electrical bring-up should use SELV continuity tests or a low-voltage test load. Applying 230/250 VAC requires an appropriately qualified setup and the completed system safety assessment.

## Validated Design State

The design was checked on 2026-08-26 with KiCad 10:

- schematic ERC: no errors or warnings;
- PCB DRC with the 8 mm custom rule: no violations;
- no unconnected pads or footprint errors;
- GPIO1 controls only K1 and GPIO2 controls only K2;
- both relay values identify `Finder 34.51.7.005.0010`;
- all 81 routed segments have assigned nets and are unique;
- no vias are used;
- one linked daughterboard-interface design-block group is present;
- all six contact nets use 1.5 mm F.Cu tracks; and
- terminal silkscreen labeling matches the intentional mirrored pin order.

This validation covers design consistency and the encoded two-dimensional PCB rules. It is not a product approval or safety certification.
