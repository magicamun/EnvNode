# EnvNode DuoRelay

`DuoRelay` is a FullSize THT daughterboard for the EnvNode Revision 0.2 module interface. It provides two independently controlled 5 V electromechanical changeover relays.

## Project Files

- `DuoRelay.kicad_pro`
- `DuoRelay.kicad_sch`
- `DuoRelay.kicad_pcb`

## Interface Use

| Connector pin | Net | DuoRelay use |
| ---: | --- | --- |
| 1 | `GND` | Control-side ground |
| 2 | `+3V3_SYS` | Identification EEPROM supply and fixed `A1` level |
| 3 | `EEPROM_A0` | Slot-defined identification EEPROM address bit |
| 4 | `+5V` | Relay-coil supply |
| 5–6 | `I2C0` | Optional identification EEPROM |
| 7–8 | `I2C1` | Not used |
| 9 | `AUX_GPIO1` | Relay channel 1 control |
| 10 | `AUX_GPIO2` | Relay channel 2 control |
| 11–14 | SPI | Not used |

Each channel uses a BC817 low-side driver with a 1 kΩ base resistor, a 100 kΩ pull-down for a defined off state, and a 1N4148W flyback diode. An LED with a 1.5 kΩ series resistor is connected from `+5V` to each transistor collector and lights while the corresponding relay is driven. The indication confirms the electrical drive state, not the mechanical position of the relay contacts. The relays are Finder `34.51.7.005.0010` parts with 5 V coils.

## Module Identification

The board provides an optional `24AA025E-OT` module-identification EEPROM and a local 100 nF decoupling capacitor on the bottom side. The EEPROM follows the EnvNode discovery convention:

- SDA and SCL use `I2C0`;
- `A1` is tied to `+3V3_SYS`;
- `A0` uses connector pin 3 / `EEPROM_A0`;
- Slot A therefore selects `0x52`; and
- Slot B selects `0x53`.

The EEPROM is not required for the relay hardware to operate, but fitting and provisioning it is good practice. Future firmware can use the identity to offer the correct relay controls in the UI and exclude incompatible functions. Automatic discovery and the required module directory are not yet implemented. Without a valid identity, firmware must treat the relay GPIOs as ordinary configurable `AUX_GPIO` ports and must not automatically start a DuoRelay-specific driver.

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

- schematic ERC reports no errors; its nine warnings comprise six intentionally unused connector nets and three same-name local/global label warnings for `+3V3_SYS`, `I2C0_SDA`, and `I2C0_SCL`;
- PCB DRC reports no violations, unconnected pads, or footprint errors; and
- connector placement, board outline, support-hole positions, power routing, relay drivers, status LEDs, and contact separation were checked against the source design.

ERC and DRC must be repeated after every schematic, footprint, routing, zone, or rule change.
