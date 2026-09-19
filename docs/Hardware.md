# EnvNode Hardware Documentation

This page is the entry point for the EnvNode hardware documentation and KiCad reference designs.

## Specifications and Guidelines

- [EnvNode Module Interface – Hardware Specification](EnvNode_Module_Interface_Hardware_Specification.md) defines the shared electrical and mechanical expansion-module interface.
- [Hardware Design Guidelines](hardware_guidelines.md) contains general design rules used across EnvNode hardware.

## KiCad Reference Designs

- `hardware/kicad/DesignBlocks/EnvNode_Module_Connector_MainBoard/` contains the mainboard-side 2 x 7 connector block.
- `hardware/kicad/DesignBlocks/EnvNode_Module_Connector_Daughterboard_SMD/` contains the bottom-mounted SMD module connector block.
- `hardware/kicad/DesignBlocks/EnvNode_Module_Connector_Daughterboard_THT/` contains the bottom-mounted THT module connector block.

## Mainboard Implementation

[`hardware/kicad/MainBoards/EnvNode Mini/`](../hardware/kicad/MainBoards/EnvNode%20Mini/README.md) implements two Revision 0.3 module slots. It also exposes `I2C0` and `I2C1` on separate 4-pin JST-SH connectors using the pin order GND, `+3V3_SYS`, SDA, SCL. Each bus has one 4.7 kΩ pull-up pair that can be connected to `+3V3_SYS` through individual, normally open solder jumpers. The jumpers allow the mainboard to assume pull-up ownership without forcing a second effective pull-up pair when another part of the assembled system already provides one. The mainboard identity is stored in a `24LC32` EEPROM on `I2C0` at address `0x50`.

## Module Templates

- `hardware/kicad/Modules/FullSize/Empty-SMD` and `Empty-THT` are the validated 38 mm x 64 mm starting points for full-size daughterboards.
- `hardware/kicad/Modules/HalfSize/Empty-SMD` and `Empty-THT` are the corresponding 38 mm x 32 mm starting points for compact daughterboards.
- All four templates place the connector axis 6.35 mm from the top edge and use M2.5 support-hole geometry.
- All four templates include the reference module-identification EEPROM circuit on the bottom side.

## Module Implementations

- [DuoRelay](../hardware/kicad/Modules/FullSize/DuoRelay/README.md) provides two `AUX_GPIO`-controlled 5 V changeover relays, one status LED per channel, and an optional identification EEPROM on a FullSize THT board.
- [AnalogHydroPressure](../hardware/kicad/Modules/FullSize/AnalogHydroPressure/README.md) is the Revision 0.3 successor to the legacy AnalogInput module. It retains the ADS1115-based 4–20 mA measurement path and generates the probe supply locally from `+5V` with an LT8330 boost converter.

The current Revision 0.3 hardware is a development baseline. Production release is gated by a complete article list and assembly data, physical bring-up of the relevant boards, and successful operation of at least the Rain Detector heater path.

## Firmware Contract Without Module Identity

Module identification is optional in the present hardware revision. If no valid module-specific EEPROM record is found, firmware must keep both I2C buses and the two slot-specific `AUX_GPIO` signals available as ordinary configurable interfaces. It must not guess a module type or start a module-specific driver automatically. SPI devices require explicit configuration as well.

When fitted, a module-identification EEPROM should use `I2C0` at the slot-defined address `0x52` or `0x53`. This is a hardware convention, not an electrical requirement. Firmware now discovers both reserved addresses, resolves supported identities through the module-profile registry, and checks board/slot compatibility. This stage remains diagnostic: automatic driver activation and module-specific UI functions are not yet implemented.
