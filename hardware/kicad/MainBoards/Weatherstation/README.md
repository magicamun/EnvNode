# EnvNode Weather

`EnvNode Weather` is the EnvNode mainboard for a fixed weather-station installation. It combines an ESP32-DevKitC, one EnvNode module slot, two externally accessible I2C buses, and dedicated GPIO/ADC sensor connectors. This directory contains the KiCad design for hardware Revision 0.1.

## Project Files

- `Weatherstation.kicad_pro`
- `Weatherstation.kicad_sch`
- `ESP32.kicad_sch`
- `gpio.kicad_sch`
- `PCB.kicad_sch`
- `Power.kicad_sch`
- `qwiic.kicad_sch`
- `SlotA.kicad_sch`

KiCad lock, backup, autosave, and local preference files are working files and must not be committed.

## Revision 0.1

Revision 0.1 uses reusable hierarchical design blocks for the board geometry, ESP32 core, power carrier, and module slot. The 5 V input protection, regulator, and 3.3 V source ORing are provided by the castellated `ORing Power` module (`U1`). The ESP32 core contains the `24LC32` mainboard-identity EEPROM (`U2`) on `I2C0` at address `0x50` and its local 100 nF decoupling capacitor (`C1`).

The module connector `J1` implements the shared EnvNode module interface. Connector pin 3 is tied to GND, assigning an optional module EEPROM in Slot A to address `0x52`.

Each I2C bus is available on four top-entry, four-pin JST-SH connectors. Individually configurable 4.7 kOhm pull-ups are normally disconnected through solder jumpers. Ten top-entry JST-PH connectors expose `ADC1` through `ADC4` and `GPIO14`, `GPIO16`, `GPIO17`, `GPIO27`, `GPIO32`, and `GPIO4`. Their common pin order is GND, `+5V`, signal, `+3V3_SYS`.

The board itself does not carry a USB or raw 5 V input connector. Power is connected to the fitted ORing Power module; its assembly and input-source rules apply to the complete unit. The module's `+3V3_EXT` interface pin is intentionally not connected on this mainboard revision.

The PCB outline and enclosure holes use the `BOX4U-5U310700` geometry. H1 and H2 are M2.5 module-support holes; H3 through H6 are the 4.5 mm enclosure mounting holes.

## Module GPIO assignment

Slot A uses GPIO33 (ADC1_CH5) on pin 9 / AUX_GPIO1 and GPIO13 on pin 10 / AUX_GPIO2. AUX_GPIO1 supports digital input, digital output/PWM, or analog input with Wi-Fi active. GPIO4 replaces GPIO33 on the separate JST-PH connector. GPIO32 remains on its separate connector; ADC1 through ADC4 remain GPIO34, GPIO35, GPIO36, and GPIO39. Firmware mappings must reflect the swap.

The current slot wiring has I2C0 SCL on pin 5 and SDA on pin 6. This differs from the published interface and AnalogHydroPressure; see the interface specification compatibility note before attaching that module.

## Validation

Run schematic ERC, PCB DRC, and schematic-PCB parity checks after every schematic, footprint, routing, zone, or rule change. Revision 0.1 still requires physical bring-up and production validation; the KiCad consistency checks do not replace electrical, thermal, EMC, environmental, or manufacturing tests. The assembly and bring-up procedure is in `Documentation/EnvNode_Weather_Bestueckungs_und_Bringup_Rev0.1.pdf`.

See the [hardware overview](../../../../docs/Hardware.md), [module-interface specification](../../../../docs/EnvNode_Module_Interface_Hardware_Specification.md), [ORing Power documentation](../../DesignBlocks/ORing%20Power/README.md), and [board identity record](../../../../docs/BoardIdentityRecord.md).
