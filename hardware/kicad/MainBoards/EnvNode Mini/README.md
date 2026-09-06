# EnvNode Mini

`EnvNode Mini` is the compact EnvNode mainboard with two module slots, ESP32 processing, dual I²C access, and configurable I²C pull-ups. This directory contains the KiCad design for hardware Revision 0.3.

## Project Files

- `EnvNode Mini.kicad_pro`
- `EnvNode Mini.kicad_sch`
- `EnvNode Mini.kicad_pcb`
- `SlotA.kicad_sch`
- `SlotB.kicad_sch`

KiCad lock and local preference files are working files and must not be committed.

## Revision 0.3

Revision 0.3 replaces the former `24AA025E48` mainboard-identity EEPROM with a `24LC32` on `I2C0` at address `0x50`. Address inputs `A0` through `A2` and write-protect `WP` are tied to GND. A local 100 nF capacitor (`C7`) decouples the EEPROM's `+3V3_SYS` supply.

The two module slots continue to implement the shared EnvNode module interface. `I2C0` and `I2C1` are also available on separate four-pin JST-SH connectors with configurable pull-ups.

## Validation

Run schematic ERC and PCB DRC after every schematic, footprint, routing, zone, or rule change. Revision 0.3 still requires physical bring-up and production validation; the KiCad consistency checks do not replace electrical, thermal, EMC, or manufacturing tests.

See the [hardware overview](../../../../docs/Hardware.md), [module-interface specification](../../../../docs/EnvNode_Module_Interface_Hardware_Specification.md), and [board identity record](../../../../docs/BoardIdentityRecord.md).
