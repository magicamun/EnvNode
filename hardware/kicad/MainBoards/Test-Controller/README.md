# Test Controller — revision 0.2

ESP32-DevKitC controller for passive mainboard continuity tests and a separate
I²C provisioning connection. Used with the [Input Adapter](../../Adapter/Input-Adapter/README.md)
and [Module Adapter](../../Adapter/Module-Adapter/README.md). This project supersedes the
controller/output section of the former monolithic Test-Provisioning board.

## Components and connections

- **U3:** removable 38-pin ESP32 DevKitC. GPIO34 reads COM_IN; GPIO17 drives
  COM_OUT through **R6, 1 kΩ**. **R9, 2.2 kΩ**, and **C6, 100 nF**, connect
  COM_IN to controller GND. Input multiplexer COMs arrive through J8 pin 9.
- **U1:** CD74HC4067 for one four-contact GPIO pigtail on GPIO1/GPIO2, one four-contact
  I²C test pigtail on I2C1/I2C2, and the two-contact reset pigtail on RST1/RST2. These are test channels, not permanently powered ports.
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
| GPIO1/GPIO2 pins 1–4 | U1 I8, I9, I10, I11 |
| I2C1/I2C2 pins 1–4 | U1 I7, I6, I5, I4 |
| RST1/RST2 pin 1: EN | U1 I15 |
| RST1/RST2 pin 2: DUT GND | U1 I0 |
| Module J3 pins 1–7 | U2 I7, I6, I5, I4, I3, I2, I1 |
| Module J3 pins 8–14 | U2 I14, I13, I12, I11, I10, I9, I8 |

The GPIO pigtail DUT contact order is GND, +5V, signal, +3V3_SYS. The I²C test
pigtail order is GND, +3V3_SYS, SDA, SCL. J3 connects pin-for-pin through a 2 × 7
IDC cable to Module Adapter J4. Arbitrary mux addresses are resolved in firmware.
Keep unselected input and output multiplexers disabled before changing addresses.

## Separate provisioning I²C

The **I2C0_1/I2C0_2** connectors uses pins 1 GND, 2 3V3_SYS, 3 SDA (**GPIO16**), 4 SCL
(**GPIO5**). It bypasses the test multiplexers. J5 enables the SDA pull-up through
R8 (4.7 kΩ); J6 enables the SCL pull-up through R7 (4.7 kΩ). This connector can
power the target and must be disconnected during passive continuity testing.
Provisioning and continuity are separate operating phases.

## Firmware and validation status

The `testboard` firmware supports real passive continuity testing, guided
loopback self-tests, simulated measurements and separate board/module provisioning.
The reset pigtail adds two output paths; it does not consume input-adapter channels.
RST1/RST2 pin 2 is a **measurement contact**, not permanent controller GND.
Do not connect the two reset contacts together during the continuity test.
The target must remain unpowered, without ESP32, and with provisioning disconnected.

This saved controller PCB passes KiCad 10.0.5 ERC, DRC and schematic parity
(2026-11-09): no violations, no unrouted connections, no mismatches. The
new connector arrangement still requires physical validation.

## Revision 0.2

Uses the Weather-size enclosure template. The additional TP2 header is:
1 GND, 2 TEST_DRIVE (after R6), 3 COM_IN. Select Controller 0.2 in the tester for
TP2 self-test instructions. GPIO assignments and output mux channel mapping remain
compatible with 0.1. CAD reviewed; physical acceptance of 0.2 remains pending.

Adapters now live in `hardware/kicad/Adapter/`. The produced Input-Adapter 0.1
remains available as Legacy; the new ESP32 DevKitC adapter is revision 0.3.
