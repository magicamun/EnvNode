# EnvNode AnalogHydroPressure

`AnalogHydroPressure` is a FullSize daughterboard for a two-wire 4–20 mA hydrostatic pressure probe. It is the Revision 0.3 successor to the legacy `AnalogInput` module.

## Project Files

- `AnalogHydroPressure.kicad_pro`
- `AnalogHydroPressure.kicad_sch`
- `AnalogHydroPressure.kicad_pcb`
- `Analog.kicad_sch` — analog input and ADS1115
- `24VBoost.kicad_sch` — local probe-supply generation
- `Documentation/AnalogHydroPressure_Bestueckungs_und_Bringup_Rev0.4.pdf` — assembly, configuration, and bring-up guide

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

At the documented design checkpoint, schematic ERC and PCB DRC completed without errors or violations, schematic/PCB parity was clean, and the layout received a visual plausibility review. These checks do not replace physical power-supply bring-up, thermal and EMC verification, probe calibration, or production testing.

The Revision 0.4 article list and assembly/bring-up guide are available. Physical boost-converter validation, thermal and EMC verification, probe calibration, and production testing remain required before production release.

## Connector review — 2026-09-28

The module uses J1 pin 5 for I2C0 SDA and pin 6 for I2C0 SCL. Mini Revision 0.7 now matches this assignment. WeatherStation still wires these two pins oppositely and requires correction; see the module-interface specification. I2C1 remains pin 7 SDA and pin 8 SCL.

J1 pin 9 / AUX_GPIO1 drives LT8330 EN/UVLO and has a pull-down to GND. J1 pin 10 / AUX_GPIO2 is unused. The new mainboard ADC1 capability on pin 9 does not make this module pin a free analog input. A future SCT013 extension should relocate boost enable to AUX_GPIO2 before using AUX_GPIO1 for its conditioned analog signal. No SCT013 circuit or such relocation is implemented by this review.

The review found no PCB DRC violations, unconnected pads, or schematic/PCB parity issues. Schematic ERC reports zero errors and ten warnings (unused labelled signals and duplicate local/global label names). These checks do not resolve the remaining WeatherStation I2C0 mismatch.
