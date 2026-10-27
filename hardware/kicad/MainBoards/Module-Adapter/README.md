# Module Adapter — revision 0.1

Passive test adapter between the [Test Controller](../Test-Controller/README.md)
module-test output and a DUT module slot. This is not a functional module template:
there is no identification EEPROM, sensor, actuator or local power source.

- **J4:** top-side 2 × 7 IDC header, 2.54 mm pitch, connected pin-for-pin to
  Controller J3 using the matching keyed ribbon cable.
- **J1:** bottom-mounted 2 × 7 THT pin header for the DUT module socket, using
  `EnvNode-Footprints:PinHeader_2x07_P2.54mm_Vertical_Bottom`.
- **H1:** 2.7 mm mounting hole for mechanical support.
- All 14 channels connect **J4.n → J1.n** without remapping. The copper plane
  belongs only to module test contact 1 (GND); it is not an independent supply return.

## Pin contract

| Pin | DUT interface signal |
| ---: | --- |
| 1 | GND |
| 2 | +3V3_SYS |
| 3 | EEPROM_A0 (slot-dependent address strap) |
| 4 | +5V |
| 5 | I2C0_SDA |
| 6 | I2C0_SCL |
| 7 | I2C1_SDA |
| 8 | I2C1_SCL |
| 9 | AUX_GPIO1 |
| 10 | AUX_GPIO2 |
| 11 | SPI_MOSI |
| 12 | SPI_MISO |
| 13 | SPI_SCK |
| 14 | SPI_CS |

These are test contact names, not rails driven continuously by the adapter.
The selected DUT profile determines slot-specific GPIOs and whether EEPROM_A0
is tied to GND or 3V3_SYS. All contacts must remain separately selectable at the
Controller output multiplexer. Check pin-1 orientation at both ribbon connectors
and at the bottom-mounted DUT plug; use the saved pad numbering rather than
inferring numbering from a top-view image.

Continuity testing requires an unpowered DUT, removed DUT ESP32 and disconnected
provisioning cable. Move the one module adapter between slots only between tests.
See the [module-interface specification](../../../../docs/EnvNode_Module_Interface_Hardware_Specification.md).

The project footprint library path is relative to this directory:
`${KIPRJMOD}/../../Libraries/EnvNode-Footprints.pretty`. Rerun ERC/DRC after any
connector, library or routing change. Physical fit and electrical behavior still
require validation on the assembled hardware.

EnvNode-owned files follow the repository LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
