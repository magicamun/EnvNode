# ESP32 DevKitC Adapter — revision 0.3

EnvNode-owned material: see the repository LICENSE.

40 measurement inputs: J2 has 38 socket pins (2×19, pitch 2.54 mm, row spacing
25.40 mm), TP1/2 is MB_3V3_SYS and TP1/1 is MB_GND. J1 connects pin-for-pin to
[Test Controller](../../MainBoards/Test-Controller/README.md) J8.

Select **ESP32 DevKitC Adapter 0.3** in tester firmware before measuring. Its mapping
differs from the [Legacy Input Adapter](../Input-Adapter/README.md). Both controller
revisions are supported. The generated mapping and PCB SHA-256 are recorded in
`firmware/testboard/fixture-esp32-devkitc0.3.json` at the repository root.

Supply 3V3_SYS/GND through J1. Keep all DUT/reference contacts isolated from supply
GND. Remove the target ESP32 and all target power before passive testing.

## Revision status

CAD and electrical routing reviewed; physical commissioning remains pending.
The title text may still mention S3; the saved J2 footprint is the 38-contact
ESP32 DevKitC variant. No 44-contact S3 adapter profile is enabled in the firmware.
