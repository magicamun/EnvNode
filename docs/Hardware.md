# EnvNode Hardware Documentation

This page is the entry point for the EnvNode hardware documentation and KiCad reference designs.

## Specifications and Guidelines

- [EnvNode Module Interface – Hardware Specification](EnvNode_Module_Interface_Hardware_Specification.md) defines the shared electrical and mechanical expansion-module interface.
- [Hardware Design Guidelines](hardware_guidelines.md) contains general design rules used across EnvNode hardware.

## KiCad Reference Designs

- `hardware/kicad/DesignBlocks/EnvNode_Module_Connector_MainBoard/` contains the mainboard-side 2 x 7 connector block.
- `hardware/kicad/DesignBlocks/EnvNode_Module_Connector_Daughterboard_SMD/` contains the bottom-mounted SMD module connector block.
- `hardware/kicad/DesignBlocks/EnvNode_Module_Connector_Daughterboard_THT/` contains the bottom-mounted THT module connector block.

## Module Templates

- `hardware/kicad/Modules/FullSize/Empty-SMD` and `Empty-THT` are the validated 38 mm x 64 mm starting points for full-size daughterboards.
- `hardware/kicad/Modules/HalfSize/Empty-SMD` and `Empty-THT` are the corresponding 38 mm x 32 mm starting points for compact daughterboards.
- All four templates place the connector axis 6.35 mm from the top edge and use M2.5 support-hole geometry.
- All four templates include the reference module-identification EEPROM circuit on the bottom side.

## Module Implementations

- [DuoRelay](../hardware/kicad/Modules/FullSize/DuoRelay/README.md) provides two `AUX_GPIO`-controlled 5 V changeover relays and an optional identification EEPROM on a FullSize THT board.
- The Revision 0.1 AnalogInput module has not yet been ported. Its former dependency on interface-provided 24 V is incompatible with the Revision 0.2 connector; a replacement must generate 24 V locally from 5 V or use a separate supply connection.

## Firmware Contract Without Module Identity

Module identification is optional in the present hardware revision. If no valid module-specific EEPROM record is found, firmware must keep both I2C buses and the two slot-specific `AUX_GPIO` signals available as ordinary configurable interfaces. It must not guess a module type or start a module-specific driver automatically. SPI devices require explicit configuration as well.

When fitted, a module-identification EEPROM should use `I2C0` at the slot-defined address `0x52` or `0x53`. This is a hardware convention, not an electrical requirement. Automatic `I2C0` discovery and the module directory required to translate an identity into drivers, capabilities, and UI behavior are not yet implemented.
