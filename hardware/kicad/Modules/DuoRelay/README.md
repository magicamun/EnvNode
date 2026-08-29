# EnvNode DuoRelay

`DuoRelay` is a FullSize THT daughterboard for the EnvNode Revision 0.2 module interface. It provides two independently controlled 5 V electromechanical changeover relays.

## Project Files

- `DuoRelay.kicad_pro`
- `DuoRelay.kicad_sch`
- `DuoRelay.kicad_pcb`

## Interface Use

| Connector pin | Net | DuoRelay use |
| ---: | --- | --- |
| 1, 3 | `GND` | Control-side ground |
| 2 | `+3V3_SYS` | Not used |
| 4 | `+5V` | Relay-coil supply |
| 5–8 | I²C buses | Not used |
| 9 | `AUX_GPIO1` | Relay channel 1 control |
| 10 | `AUX_GPIO2` | Relay channel 2 control |
| 11–14 | SPI | Not used |

Each channel uses a BC817 low-side driver with a 1 kΩ base resistor, a 100 kΩ pull-down for a defined off state, and a 1N4148W flyback diode. The relays are Finder `34.51.7.005.0010` parts with 5 V coils.

## Module Identification

This revision has no module-identification EEPROM. Firmware must therefore treat the slot as unidentified and must not automatically start a DuoRelay-specific driver. The relay GPIOs are ordinary configurable `AUX_GPIO` ports until the user explicitly assigns their function.

## Mechanical Geometry

- PCB: 38.00 mm × 64.00 mm
- Connector: bottom-mounted 2 × 7 THT header
- Connector axis: centered across the board and 6.35 mm from the top edge
- Support holes: 2.7 mm for M2.5 hardware, on the centerline at 27.00 mm and 59.00 mm from the top edge

## Relay Contact Region

The relay-contact nets are routed on F.Cu with 1.0 mm tracks. The design keeps at least 8 mm between the 250 VAC contact region and all SELV copper on both layers, including GND zones, vias, pads, and traces.

The spacing and relay ratings do not by themselves certify the finished product for mains use. Terminal ratings, enclosure, touch protection, fusing, load type, pollution degree, material group, overvoltage category, manufacturing tolerances, and the applicable product standard must be assessed for the final application.

## Validation

At the time of the Revision 0.2 port:

- schematic ERC reports no errors; the remaining warnings refer only to intentionally unused connector nets;
- PCB DRC reports no violations, unconnected pads, or footprint errors; and
- connector placement, board outline, support-hole positions, power routing, relay drivers, and contact separation were checked against the source design.

ERC and DRC must be repeated after every schematic, footprint, routing, zone, or rule change.
