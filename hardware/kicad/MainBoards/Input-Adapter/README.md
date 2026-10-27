# Input Adapter — revision 0.1

Passive-DUT measurement adapter for the [Test Controller](../Test-Controller/README.md).
Three CD74HC4067 multiplexers select one of **40 independent measurement inputs**:
38 ESP32 socket contacts plus MB_3V3_SYS and MB_GND. The adapter contains no ESP32.

## Connectors and power

- **J4:** DUT test plug, 2 × 19 pins, 2.54 mm pitch, 25.40 mm row spacing. From the
  top, left pins run 1–19 and right pins run 38–20. Remove the DUT ESP32 before use.
- **J7:** two-pin header for the reference pigtail: pin 1 MB_3V3_SYS, pin 2 MB_GND.
  The cable terminates in the agreed two-pin JST-PH on a suitably revised mainboard.
  Both are measurement contacts, not adapter supply connections.
- **J1:** 2 × 5 IDC header, 2.54 mm pitch. Connect pin-for-pin to Controller J8.
  Use matching pin-1 markings and keyed cable assembly; do not assume orientation
  from connector placement on the PCB.
- J1 supplies 3V3_SYS and GND. C1–C3 provide 100 nF local decoupling for U4–U6.
  MB_GND and all DUT socket contacts remain separate from adapter supply GND.
- COM is shared by U4–U6. Enable only one multiplexer at a time. Active-low enable
  pull-ups and the ADC pulldown/filter are on the Controller, not this adapter.

## J1 cable contract

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

## Input routing contract

Logical inputs 1–38 retain the DUT socket numbering. Proposed logical inputs 39 and
40 are the added reference contacts; these are not additional ESP32 socket pins.
I0–I15 denote analog multiplexer channels; S0 is the least-significant address bit.
The table records the saved PCB wiring, not a presumed sequential layout.

| Logical input | Connector.pad | Net | Multiplexer | Channel | IC pad |
| ---: | --- | --- | --- | --- | ---: |
| 1 | J4.1 | L01 | U4 | I7 | 2 |
| 2 | J4.2 | L02 | U4 | I6 | 3 |
| 3 | J4.3 | L03 | U4 | I5 | 4 |
| 4 | J4.4 | L04 | U4 | I4 | 5 |
| 5 | J4.5 | L05 | U4 | I3 | 6 |
| 6 | J4.6 | L06 | U4 | I2 | 7 |
| 7 | J4.7 | L07 | U4 | I1 | 8 |
| 8 | J4.8 | L08 | U4 | I0 | 9 |
| 9 | J4.9 | L09 | U5 | I7 | 2 |
| 10 | J4.10 | L10 | U5 | I6 | 3 |
| 11 | J4.11 | L11 | U5 | I5 | 4 |
| 12 | J4.12 | L12 | U5 | I4 | 5 |
| 13 | J4.13 | L13 | U5 | I3 | 6 |
| 14 | J4.14 | L14 | U5 | I2 | 7 |
| 15 | J4.15 | L15 | U6 | I7 | 2 |
| 16 | J4.16 | L16 | U6 | I6 | 3 |
| 17 | J4.17 | L17 | U6 | I5 | 4 |
| 18 | J4.18 | L18 | U6 | I4 | 5 |
| 19 | J4.19 | L19 | U6 | I3 | 6 |
| 20 | J4.20 | R19 | U6 | I12 | 19 |
| 21 | J4.21 | R18 | U6 | I11 | 20 |
| 22 | J4.22 | R17 | U6 | I10 | 21 |
| 23 | J4.23 | R16 | U6 | I9 | 22 |
| 24 | J4.24 | R15 | U6 | I8 | 23 |
| 25 | J4.25 | R14 | U5 | I13 | 18 |
| 26 | J4.26 | R13 | U5 | I12 | 19 |
| 27 | J4.27 | R12 | U5 | I11 | 20 |
| 28 | J4.28 | R11 | U5 | I10 | 21 |
| 29 | J4.29 | R10 | U5 | I9 | 22 |
| 30 | J4.30 | R09 | U5 | I8 | 23 |
| 31 | J4.31 | R08 | U4 | I15 | 16 |
| 32 | J4.32 | R07 | U4 | I14 | 17 |
| 33 | J4.33 | R06 | U4 | I13 | 18 |
| 34 | J4.34 | R05 | U4 | I12 | 19 |
| 35 | J4.35 | R04 | U4 | I11 | 20 |
| 36 | J4.36 | R03 | U4 | I10 | 21 |
| 37 | J4.37 | R02 | U4 | I9 | 22 |
| 38 | J4.38 | R01 | U4 | I8 | 23 |
| 39 | J7.1 | MB_3V3_SYS | U6 | I13 | 18 |
| 40 | J7.2 | MB_GND | U6 | I14 | 17 |

## Operation and firmware status

Continuity testing requires an unpowered DUT, its ESP32 removed, and the separate
provisioning cable disconnected. Connect the reference pigtail only to a compatible
DUT revision. Mainboard-side series protection diodes make the reference paths
unidirectional; thresholds and test current direction require physical validation.

The current test firmware still simulates 38 socket inputs. Extending its model,
profile data, comparisons, UI and tests to 40 inputs remains a firmware task.
The new references must not be silently treated as present on old mainboards.
Physical I/O, switching sequence and ADC settling/thresholds remain unvalidated.

The monolithic Test-Provisioning project is superseded by Input-Adapter,
Test-Controller and Module-Adapter. Earlier revisions remain in Git history.

EnvNode-owned files follow the repository LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
