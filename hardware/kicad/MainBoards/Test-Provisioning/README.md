# Test and provisioning board

ESP32-DevKitC-based fixture for passive mainboard connection tests and a separate I2C provisioning connection.

- J4: 38 independent contacts, DevKitC numbering, 2.54 mm pitch and 25.40 mm row spacing. U4-U6 select the measured contact.
- U1: GPIO/I2C test pigtails. U2: 14-contact module test adapter.
- GPIO17 drives the output COMs through R6 (1 kOhm). GPIO34 reads the input COMs with R9 (2.2 kOhm) and C6 (100 nF) to ground.
- Each active-low multiplexer enable has a 10 kOhm pull-up; each multiplexer has 100 nF supply decoupling.
- Separate provisioning I2C: GPIO16 SDA, GPIO5 SCL; J5/J6 enable the 4.7 kOhm pull-ups individually.
- ORing power module supplies 3V3_SYS; DevKit 3V3 is not directly tied to this rail.

Continuity testing requires an unpowered DUT, removed DUT ESP32, and disconnected provisioning cable. ADC thresholds and settling times require validation on physical hardware, including parallel pull-up paths. Firmware and the module-slot daughterboard are separate work.

Validation on 2026-10-25: KiCad CLI ERC: 0 violations; PCB DRC: 0 violations, 0 unconnected items, 0 schematic parity issues, under the saved project settings. Configured exclusions/ignored rules remain in effect. No physical assembly or measurement validation has been performed.

EnvNode-owned files follow the repository LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
