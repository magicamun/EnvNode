# EnvNode Module Interface – Hardware Specification

**Status:** Implemented design baseline

**Revision:** 0.7

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
| 3 | `GND` | Ground |
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

| Slot | Pin 9 | Pin 10 | Pin 14 |
| --- | --- | --- | --- |
| A | `AUX1` | `AUX2` | `CS` |
| B | `AUX3` | `AUX4` | `AUX6` used as the second chip select |

MOSI, MISO, and SCK are shared between slots. Each populated SPI module requires its own chip-select signal. On the two-slot Mini this consumes the core `CS` signal for Slot A and `AUX6` for Slot B.

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

- `hardware/kicad/Modules/EmptyHalf_SMD/`
- `hardware/kicad/Modules/EmptyHalf_THT/`
- `hardware/kicad/Modules/EmptyFull_SMD/`
- `hardware/kicad/Modules/EmptyFull_THT/`

The source project and the separately published schematic and PCB portions in `hardware/kicad/Libraries/EnvNodeDesignBlocks.kicad_blocks/` must be kept synchronized. Updating a schematic from a design block does not automatically replace an already placed PCB layout.

## 7. Module Identification and Generic Fallback

A module may later contain a module-specific identification EEPROM, but such an EEPROM is not mandatory in this revision. DuoRelay intentionally has none.

If the firmware finds no valid module-specific identity, it must not infer a module type from the PCB or automatically activate a device-specific driver. The slot remains in generic mode:

- both I²C buses remain available as ordinary buses; and
- the two `AUX_GPIO` signals remain ordinary configurable GPIO ports.

SPI operation likewise requires explicit configuration because the absence of a module identity provides no information about an attached SPI device or its protocol.

The mainboard identity EEPROM is independent of optional module identification. Mainboard Revision 0.2 uses the `24AA025E48` at I²C address `0x50` on `I2C0`; its record is defined in `docs/BoardIdentityRecord.md`.

## 8. DuoRelay Reference Module

`hardware/kicad/Modules/DuoRelay/` is the first module ported to the Revision 0.2 interface. It is a FullSize THT module with two independently controlled 5 V changeover relays:

- connector pin 9 / `AUX_GPIO1` drives relay channel 1;
- connector pin 10 / `AUX_GPIO2` drives relay channel 2;
- each channel uses a BC817 low-side driver, 1 kΩ base resistor, 100 kΩ pull-down, and 1N4148W flyback diode; and
- the SPI and I²C pins are unused by the present hardware.

The relay-contact routing uses 1.0 mm traces. The 250 VAC contact region is kept at least 8 mm from all SELV copper on both copper layers, including the GND zone. This project-specific layout decision is not a blanket safety certification; enclosure, terminals, fusing, pollution degree, material group, overvoltage category, load type, and applicable product standards still belong to the completed product assessment.

## 9. Open Points

The following remain intentionally open:

1. Exact production connector part numbers and qualified mated stack height.
2. Per-rail current limits and a complete mainboard/module power budget.
3. I²C pull-up ownership, bus capacitance limits, and supported clock rates.
4. Electrical protection requirements for modules exposed to external wiring.
5. The format, address allocation, and discovery procedure for a future module-identification EEPROM.
6. Firmware implementation of the generic unidentified-module mode.

## 10. Revision Status

Revision 0.7 replaces the obsolete two-connector 2 × 4 definition with the implemented single-connector 2 × 7 interface. It fixes the pin assignment, records that 24 V is not present, documents the two-slot EnvNode Mini mapping and shared-SPI chip-select rule, defines the HalfSize and FullSize reference geometry, and records the generic firmware behavior for modules without an identification EEPROM.
