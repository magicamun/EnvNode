# EnvNode Module Interface – Hardware Specification

**Status:** Implemented design baseline

**Revision:** 0.9

**Applies to:** Mainboard Revision 0.2 and later

## 1. Purpose and Scope

The EnvNode Module Interface is the internal electrical and mechanical interface between an EnvNode mainboard and a plug-in module. It supports I²C, SPI, two slot-specific auxiliary GPIOs, and the 3.3 V and 5 V system rails.

The interface deliberately does **not** carry 24 V. A module that requires 24 V must generate it locally from 5 V or receive it through a separate, application-specific connection.

Backward compatibility with the earlier Revision 0.1 two-connector interface is not a requirement.

## 2. Connector and Pin Assignment

Each physical module slot uses one 2 × 7 connector on a 2.54 mm pitch with odd/even pin numbering.

| Pin | Interface net | Function |
| ---: | --- | --- |
| 1 | `GND` | Ground |
| 2 | `+3V3_SYS` | 3.3 V system rail |
| 3 | `EEPROM_A0` | Slot-defined module-identification EEPROM address bit |
| 4 | `+5V` | 5 V system rail |
| 5 | `I2C0_SDA` | System I²C data |
| 6 | `I2C0_SCL` | System I²C clock |
| 7 | `I2C1_SDA` | Secondary I²C data |
| 8 | `I2C1_SCL` | Secondary I²C clock |
| 9 | `AUX_GPIO1` | First slot-specific GPIO |
| 10 | `AUX_GPIO2` | Second slot-specific GPIO |
| 11 | `SPI_MOSI` | Shared SPI controller-to-module data |
| 12 | `SPI_MISO` | Shared SPI module-to-controller data |
| 13 | `SPI_SCK` | Shared SPI clock |
| 14 | `SPI_CS` | Slot-specific SPI chip select |

The generic names `AUX_GPIO1`, `AUX_GPIO2`, and `SPI_CS` describe the connector contract. A mainboard maps them to distinct resources for each slot.

Unused pins may remain electrically unconnected on a module but must not be repurposed.

## 3. Mainboard Revision 0.2 Mapping

The ESP32 Core Dual Power design fixes the shared buses as follows:

| Interface signal | ESP32 GPIO |
| --- | ---: |
| `I2C0_SDA` | GPIO21 |
| `I2C0_SCL` | GPIO22 |
| `I2C1_SDA` | GPIO25 |
| `I2C1_SCL` | GPIO26 |
| `SPI_MOSI` | GPIO23 |
| `SPI_MISO` | GPIO19 |
| `SPI_SCK` | GPIO18 |
| Slot A `SPI_CS` | GPIO5 / core net `CS` |

EnvNode Mini implements two slots:

| Slot | Pin 3 / `EEPROM_A0` | Pin 9 | Pin 10 | Pin 14 |
| --- | --- | --- | --- | --- |
| A | `GND` | `AUX1` | `AUX2` | `CS` |
| B | `+3V3_SYS` | `AUX3` | `AUX4` | `AUX6` used as the second chip select |

MOSI, MISO, and SCK are shared between slots. Each populated SPI module requires its own chip-select signal. On the two-slot Mini this consumes the core `CS` signal for Slot A and `AUX6` for Slot B.

EnvNode Mini additionally exposes both I²C buses for direct cable connection:

| Connector | Bus | Pin 1 | Pin 2 | Pin 3 | Pin 4 |
| --- | --- | --- | --- | --- | --- | --- |
| `J4` | `I2C0` | GND | `+3V3_SYS` | SDA | SCL |
| `J5` | `I2C1` | GND | `+3V3_SYS` | SDA | SCL |

Both connectors use the horizontal JST-SH `SM04B-SRSS-TB` footprint with 1.00 mm pitch. Each bus has a dedicated pair of 4.7 kΩ pull-up resistors. Four normally open solder jumpers connect the pull-ups individually to `+3V3_SYS`; closing both jumpers for a bus makes EnvNode Mini the pull-up owner for that bus. They must remain open if the assembled bus already has an effective pull-up pair.

## 4. Power

The mainboard supplies `+5V` and `+3V3_SYS` to each slot. Modules are consumers of these rails unless a future module and mainboard specification explicitly defines another power-flow direction and the required protection.

`+3V3_SYS` is the already ORed system rail. A module must not connect another uncoordinated 3.3 V source to it.

The connector specification does not itself define an allowable current. Every module must be checked against the selected connector, copper thickness, trace widths, thermal conditions, mainboard supply budget, and expected simultaneous load.

## 5. Mechanical Definition

The reference daughterboards are 38.00 mm wide. Their connector axis is centered across the board width and is 6.35 mm from the top board edge.

| Template | PCB size | Support-hole axis from top |
| --- | --- | ---: |
| HalfSize | 38.00 mm × 32.00 mm | 27.00 mm |
| FullSize | 38.00 mm × 64.00 mm | 27.00 mm and 59.00 mm |

Support holes use 2.7 mm drill geometry for common M2.5 hardware. The installed spacer height must match the mated connector stack height.

The module connector is mounted on the module's bottom side. SMD and THT reference implementations exist; their pad numbering must produce the same assembled pin-to-pin mapping as the mainboard socket.

## 6. KiCad Reference Implementations

Current source design blocks:

- `hardware/kicad/DesignBlocks/EnvNode_Module_Connector_MainBoard/`
- `hardware/kicad/DesignBlocks/EnvNode_Module_Connector_Daughterboard_SMD/`
- `hardware/kicad/DesignBlocks/EnvNode_Module_Connector_Daughterboard_THT/`

Current empty module templates:

- `hardware/kicad/Modules/FullSize/Empty-SMD/`
- `hardware/kicad/Modules/FullSize/Empty-THT/`
- `hardware/kicad/Modules/HalfSize/Empty-SMD/`
- `hardware/kicad/Modules/HalfSize/Empty-THT/`

The source project and the separately published schematic and PCB portions in `hardware/kicad/Libraries/EnvNodeDesignBlocks.kicad_blocks/` must be kept synchronized. Updating a schematic from a design block does not automatically replace an already placed PCB layout.

## 7. Module Identification and Generic Fallback

A module-identification EEPROM is optional, but fitting one is good practice for modules with a defined hardware identity. It allows future firmware to identify the installed module, offer suitable functions in the UI, and suppress functions that are incompatible with the detected hardware. The Empty templates and DuoRelay provide the reference implementation.

A conforming module-identification EEPROM should use `I2C0`. This is a discovery convention rather than an electrical limitation: both I²C buses remain available at the connector, but future automatic module discovery will scan only `I2C0`. The firmware scan and the module directory that maps stored identities to drivers, capabilities, and UI behavior are not yet implemented.

The reference EEPROM uses two address bits. Its module-side wiring is:

- `A1` tied to `+3V3_SYS`;
- `A0` connected to connector pin 3 / `EEPROM_A0`; and
- a 100 nF decoupling capacitor placed close to the EEPROM supply pins.

The mainboard supplies the slot-dependent `A0` level, producing the reserved `I2C0` discovery addresses:

| Slot | A1 | A0 | EEPROM address |
| --- | ---: | ---: | ---: |
| A | 1 | 0 | `0x52` |
| B | 1 | 1 | `0x53` |

The addresses `0x52` and `0x53` on `I2C0` are reserved for module-identification EEPROMs. Other module devices should not use these addresses on `I2C0`.

If the firmware finds no valid module-specific identity, it must not infer a module type from the PCB or automatically activate a device-specific driver. The slot remains in generic mode:

- both I²C buses remain available as ordinary buses; and
- the two `AUX_GPIO` signals remain ordinary configurable GPIO ports.

SPI operation likewise requires explicit configuration because the absence of a module identity provides no information about an attached SPI device or its protocol.

The mainboard identity EEPROM is independent of optional module identification. Mainboard Revision 0.3 uses a `24LC32` at I²C address `0x50` on `I2C0`; `A0` through `A2` and `WP` are tied to GND. Its record is defined in `docs/BoardIdentityRecord.md`. Revision 0.2 used a `24AA025E48` at the same bus address.

## 8. DuoRelay Reference Module

`hardware/kicad/Modules/FullSize/DuoRelay/` is the first functional module ported to the Revision 0.2 interface. It is a FullSize THT module with an optional identification EEPROM and two independently controlled 5 V changeover relays:

- connector pin 9 / `AUX_GPIO1` drives relay channel 1;
- connector pin 10 / `AUX_GPIO2` drives relay channel 2;
- each channel uses a BC817 low-side driver, 1 kΩ base resistor, 100 kΩ pull-down, and 1N4148W flyback diode;
- each channel has an LED and 1.5 kΩ series resistor from `+5V` to the transistor collector, so the LED lights when that relay is driven;
- `I2C0` connects the optional module-identification EEPROM; and
- the remaining SPI and I²C pins are unused by the present functional hardware.

The relay-contact routing uses 1.0 mm traces. The 250 VAC contact region is kept at least 8 mm from all SELV copper on both copper layers, including the GND zone. This project-specific layout decision is not a blanket safety certification; enclosure, terminals, fusing, pollution degree, material group, overvoltage category, load type, and applicable product standards still belong to the completed product assessment.

## 9. Open Points

The following remain intentionally open:

1. Exact production connector part numbers and qualified mated stack height.
2. Per-rail current limits and a complete mainboard/module power budget.
3. Bus capacitance limits, supported clock rates, and the required pull-up jumper configuration for each complete system assembly.
4. Electrical protection requirements for modules exposed to external wiring.
5. The byte-level module-identity record format and the module-directory schema.
6. Firmware implementation of `I2C0` module discovery and the generic unidentified-module mode.

## 10. Revision Status

Revision 0.9 adds the EnvNode Mini JST-SH I²C access and configurable mainboard pull-ups, records the DuoRelay channel-status LEDs, and retains the Revision 0.8 module-identification convention and generic firmware fallback.
