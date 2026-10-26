# EnvNodeI2C

EnvNode-owned materials follow the repository LICENSE
(LicenseRef-EnvNode-Use-and-Redistribution-1.0).

Shared Arduino I2C transport with fixed wiring supplied by the application.
This library has no board-profile, sensor, web, NVS or runtime dependencies.
It uses EnvNodeLogging and the EnvNodeIdentity storage interface.

- I2CBusTypes: stable logical bus IDs and bus/SDA/SCL startup data.
- I2CBusController: initialization, availability, scanning and cached results.
- II2CBusProvider: narrow interface for obtaining an initialized TwoWire bus.
- IdentityEeprom24LC32: injected bus provider, device address and bus ID.

The controller uses Wire for I2C0 and Wire1 for I2C1. Repeating the same startup
assignment is idempotent; remapping an initialized bus is rejected, and failed
initialization may be retried. Pins are fixed in the application's source, not
stored in NVS or changed through the web UI.

EnvNode's I2CBusManager remains an application adapter. Its fixed identity
bootstrap is EnvNodeIdentityI2C (I2C0, SDA21, SCL22); normal startup and scanAll
use the active board profile. Existing manager entry points and EEPROM
constructor calls remain valid; the EEPROM bus defaults to I2C0 for compatibility.
EnvNode composition explicitly passes its fixed identity bus. A future test
application supplies its own constant to I2CBusController and its EEPROM objects.
It does not use EnvNode's bootstrap or active-board selection.

EEPROM capacity (4096 bytes), 32-byte pages/read chunks, address encoding,
ACK polling and 10 ms write timeout retain their previous behavior. Native tests
exercise bus/device isolation, boundary splitting and failure reporting with an
emulated Wire transport. Physical signal timing and EEPROM behavior still need
hardware validation.
