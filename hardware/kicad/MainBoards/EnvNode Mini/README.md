# EnvNode Mini

`EnvNode Mini` is the compact EnvNode mainboard with two module slots, ESP32 processing, dual I²C access, and configurable I²C pull-ups. This directory contains the KiCad design for hardware Revision 0.6.

## Project Files

- `EnvNode Mini.kicad_pro`
- `EnvNode Mini.kicad_sch`
- `EnvNode Mini.kicad_pcb`
- `SlotA.kicad_sch`
- `SlotB.kicad_sch`
- `ESP32Core.kicad_sch`
- `PCB.kicad_sch`
- `Power.kicad_sch`

KiCad lock and local preference files are working files and must not be committed.

## Revision 0.6

Revision 0.6 restructures the mainboard around reusable hierarchical design blocks. The 5 V input protection, regulator, and 3.3 V source ORing are provided by the castellated `ORing Power` module (`U2`) instead of being assembled directly on the mainboard. The ESP32 core contains the `24LC32` mainboard-identity EEPROM (`U1`) on `I2C0` at address `0x50` and its local 100 nF decoupling capacitor (`C1`).

The two module slots continue to implement the shared EnvNode module interface. Net ties set Slot A `EEPROM_A0` to GND and Slot B `EEPROM_A0` to `+3V3_SYS`, assigning the optional module EEPROM addresses `0x52` and `0x53`. Each I²C bus is available on two top-entry, four-pin JST-SH connectors. Individually configurable 4.7 kΩ pull-ups remain normally disconnected through solder jumpers.

The mainboard itself no longer carries a USB or raw 5 V input connector. Power is connected to the fitted ORing Power module; its assembly and input-source rules apply to the complete unit. The module's `+3V3_EXT` interface pin is intentionally not connected on this mainboard revision.

## Module GPIO assignment

| Slot | Pin 9 / AUX_GPIO1 | Pin 10 / AUX_GPIO2 | Pin 14 / SPI_CS |
| --- | --- | --- | --- |
| A | GPIO33 (ADC1_CH5) | GPIO13 | GPIO5 |
| B | GPIO32 (ADC1_CH4) | GPIO14 | GPIO27 |

AUX_GPIO1 supports digital input, digital output/PWM, or analog input while Wi-Fi is active. AUX_GPIO2 supports digital input and output/PWM; its ADC2 function is not available during Wi-Fi operation. GPIO4 and GPIO16 are now unconnected. Firmware slot mappings must follow this assignment.

The current slot wiring has I2C0 SCL on pin 5 and SDA on pin 6. This differs from the published interface and AnalogHydroPressure; see the interface specification compatibility note before attaching that module.

## Validation

Run schematic ERC and PCB DRC after every schematic, footprint, routing, zone, or rule change. Revision 0.6 still requires physical bring-up and production validation; the KiCad consistency checks do not replace electrical, thermal, EMC, or manufacturing tests. The assembly and bring-up procedure is in `Documentation/EnvNode_Mini_Bestueckungs_und_Bringup_Rev0.6.pdf`.

See the [hardware overview](../../../../docs/Hardware.md), [module-interface specification](../../../../docs/EnvNode_Module_Interface_Hardware_Specification.md), and [board identity record](../../../../docs/BoardIdentityRecord.md).
