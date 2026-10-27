# Test Controller — revision 0.1

ESP32-DevKitC controller for passive mainboard continuity tests and a separate
I²C provisioning connection. Used with the [Input Adapter](../Input-Adapter/README.md)
and [Module Adapter](../Module-Adapter/README.md). This project supersedes the
controller/output section of the former monolithic Test-Provisioning board.

## Components and connections

- **U3:** removable 38-pin ESP32 DevKitC. GPIO34 reads COM_IN; GPIO17 drives
  COM_OUT through **R6, 1 kΩ**. **R9, 2.2 kΩ**, and **C6, 100 nF**, connect
  COM_IN to controller GND. Input multiplexer COMs arrive through J8 pin 9.
- **U1:** CD74HC4067 for one four-contact GPIO pigtail on J1 and one four-contact
  I²C test pigtail on J2. These are test channels, not permanently powered ports.
- **U2:** CD74HC4067 for the 14 module-test channels on J3.
- **J8:** ten-wire IDC connection to Input Adapter J1, including supply and GND.
- **U7:** ORing Power module; +5V feeds the DevKit and module, 3V3_SYS powers
  the multiplexers and external adapters. DevKit 3V3 is intentionally not tied
  directly to 3V3_SYS; U7 external/DevKit 3.3 V inputs are marked NC.
- **R1–R3:** 10 kΩ pull-ups on IN_EN1–3; **R5/R4:** 10 kΩ pull-ups on
  OUT_EN1/2. All enables are active low. C5/C4 provide 100 nF decoupling at U1/U2.

## J8 to Input Adapter J1

| Pin | Signal | Controller GPIO / function |
| ---: | --- | --- |
| 1 | 3V3_SYS | Adapter supply |
| 2 | GND | Supply ground |
| 3 | IN_EN1 | GPIO14; U4 enable, active low |
| 4 | IN_S3 | GPIO13 |
| 5 | IN_EN2 | GPIO26; U5 enable, active low |
| 6 | IN_S2 | GPIO27 |
| 7 | IN_EN3 | GPIO33; U6 enable, active low |
| 8 | IN_S1 | GPIO25 |
| 9 | COM / COM_IN | GPIO34 / ADC1 |
| 10 | IN_S0 | GPIO32 |

## Output routing

| Function | GPIO / channel |
| --- | --- |
| OUT_S0, S1, S2, S3 | GPIO18, GPIO19, GPIO21, GPIO22 |
| OUT_EN1 (U1), OUT_EN2 (U2) | GPIO23, GPIO4 |
| Output drive | GPIO17 via R6, shared by U1/U2 COM |
| GPIO pigtail J1 pins 1–4 | U1 I8, I9, I10, I11 |
| I²C test pigtail J2 pins 1–4 | U1 I7, I6, I5, I4 |
| Module J3 pins 1–7 | U2 I7, I6, I5, I4, I3, I2, I1 |
| Module J3 pins 8–14 | U2 I14, I13, I12, I11, I10, I9, I8 |

The GPIO pigtail DUT contact order is GND, +5V, signal, +3V3_SYS. The I²C test
pigtail order is GND, +3V3_SYS, SDA, SCL. J3 connects pin-for-pin through a 2 × 7
IDC cable to Module Adapter J4. Arbitrary mux addresses are resolved in firmware.
Keep unselected input and output multiplexers disabled before changing addresses.

## Separate provisioning I²C

The **I2C0** header uses pins 1 GND, 2 3V3_SYS, 3 SDA (**GPIO16**), 4 SCL
(**GPIO5**). It bypasses the test multiplexers. J5 enables the SDA pull-up through
R8 (4.7 kΩ); J6 enables the SCL pull-up through R7 (4.7 kΩ). This connector can
power the target and must be disconnected during passive continuity testing.
Provisioning and continuity are separate operating phases.

## Firmware and validation status

The `testboard` PlatformIO target currently provides a guided simulation; physical
mux/ADC control and provisioning operations are not yet implemented. The split
fixture requires 40 logical inputs (38 socket pads plus two references), the J8
GPIO assignments above, and matching board-revision profiles. Do not equate
MB_GND with supply GND or MB_3V3_SYS with DevKit 3V3. Measurement thresholds,
diode effects and settling times require validation on assembled hardware.

The last review found no electrical ERC errors, open PCB connections or schematic
parity errors. Silkscreen text overlap and the ESP32 PCB-antenna clearance were
reported separately; rerun ERC/DRC and inspect antenna clearance after changes.
Only the saved project rules are checked; no physical validation has occurred.

EnvNode-owned files follow the repository LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
