# EnvNode AnalogHydroPressure

`AnalogHydroPressure` is a FullSize daughterboard for a two-wire 4–20 mA hydrostatic pressure probe. The current hardware is Revision 0.5, succeeding the legacy `AnalogInput` module.

## Project Files

- `AnalogHydroPressure.kicad_pro`
- `AnalogHydroPressure.kicad_sch`
- `AnalogHydroPressure.kicad_pcb`
- `Analog.kicad_sch` — analog input and ADS1115
- `24VBoost.kicad_sch` — local probe-supply generation
- `Documentation/AnalogHydroPressure_Bestueckungs_und_Bringup_Rev0.5.pdf` — assembly, configuration, and bring-up guide

## Functional Structure

The module has two hierarchical circuit sections:

- The analog section converts the probe loop current across a 150 ohm, 0.1% burden resistor, filters and clamps the signal, and digitizes it with an `ADS1115IDGS` on `I2C0`.
- The boost section uses an `LT8330S6` to generate the local 24 V probe rail from the module interface's `+5V` supply.

The ADS1115 address is selected with one of four solder jumpers for addresses `0x48` through `0x4B`. Exactly one address jumper must be closed before operation.

The slide-switch positions allow the incoming 5 V supply path to be selected without changing the reusable connector design block. The schematic and PCB use:

`EnvNode-Footprints:SW_Slide_CK_OS202011MS2QS1`

## Local Libraries

The LT8330 symbol is stored in:

`hardware/kicad/Libraries/EnvNode-Symbols.kicad_sym`

The C&K switch footprint is stored in:

`hardware/kicad/Libraries/EnvNode-Footprints.pretty/SW_Slide_CK_OS202011MS2QS1.kicad_mod`

The project-level `sym-lib-table` maps the symbol library. The footprint library name must resolve as `EnvNode-Footprints` in KiCad.

## Bring-up Priorities

1. Populate and inspect the 5 V input and LT8330 boost section.
2. Power it from a current-limited 5 V supply before connecting a probe.
3. Verify startup, switching waveform, output voltage, ripple, and component temperature under the intended load.
4. Verify that exactly one ADS1115 address jumper is closed and that the ADC is detected on `I2C0`.
5. Apply known loop currents and verify approximately 0.6 V at 4 mA and 3.0 V at 20 mA across the nominal 150 ohm burden.
6. Calibrate the complete measurement path with the intended pressure probe.

## Validation and Release Status

Revision 0.4 uses the B260S1F-7 boost diode, selectable ADS1115 address jumpers, and an optional DNP measurement header for GND, `+24V`, `LOOP_RETURN`, and `AIN0`. The bottom-side identification EEPROM and its decoupling capacitor remain DNP in the current kit.

At the historical design checkpoint on 2026-09-28, schematic ERC and PCB DRC completed without errors or violations, schematic/PCB parity was clean, and the layout received a visual plausibility review. These checks do not replace physical power-supply bring-up, thermal and EMC verification, probe calibration, or production testing.

The Revision 0.5 assembly/bring-up guide documents AUX_GPIO2 / connector pin 10 for boost enable, including continuity checks, startup sequencing, updated PCB views, and source hashes. Revision 0.4 remains available as a historical guide; its article list predates the enable reassignment. Physical boost-converter validation, thermal and EMC verification, probe calibration, and production testing remain required before production release.

## Connector review — 2026-09-28

The module uses J1 pin 5 for I2C0 SDA and pin 6 for I2C0 SCL. Mini Revision 0.7 and WeatherStation Revision 0.2 both match this assignment in schematic and PCB; the earlier mainboard I2C0 mismatch is resolved for these revisions. I2C1 remains pin 7 SDA and pin 8 SCL.

Revision 0.5 moves LT8330 EN/UVLO and its R7 pull-down to J1 pin 10 / AUX_GPIO2 in both schematic and PCB. J1 pin 9 / AUX_GPIO1 is now unused and available for a future conditioned SCT013 analog signal; no SCT013 circuit is implemented yet. Firmware must use AUX_GPIO2 for boost enable: GPIO13 on Mini Slot A and WeatherStation Slot A, GPIO14 on Mini Slot B. Firmware was not modified as part of this hardware revision.

The Revision 0.5 review on 2026-09-28, with zone refill, found no PCB DRC violations, unconnected pads, or schematic/PCB parity issues. Schematic ERC reports zero errors and ten warnings (unused labelled signals and duplicate local/global label names). The separate connector comparison confirms the corrected I2C0 assignment on Mini Revision 0.7 and WeatherStation Revision 0.2.

## Documentation verification — 2026-10-05

The Revision 0.5 guide was checked against an exported schematic netlist and PCB pad nets. J1.10, U4.4 (EN/UVLO), and R7.1 share AUX_GPIO2; J1.9 remains separate. The PCB views use the common ORing camera angles with preserved proportions. No new full ERC/DRC run or physical validation was performed during this documentation update.
