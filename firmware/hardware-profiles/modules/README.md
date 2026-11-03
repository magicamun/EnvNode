# SD-Card profile integration staging

EnvNode-owned content: see repository LICENSE
(LicenseRef-EnvNode-Use-and-Redistribution-1.0).

`sd-card-0.1.json` follows the unified profile wrapper currently maintained in
main, alongside `duo-relay-0.3.json`. This branch does not yet contain that
profile generator or its provisioning catalogue. The file is a prepared source
profile, not an activated firmware integration or a provisioned EEPROM.

The descriptor identifies PCB revision 0.1 and requests SPI, slot-specific
SPI_CS, AUX_GPIO1 card detect (HIGH means inserted) and +5V. The 24LC32 identity
EEPROM uses the module interface's existing slot-dependent addressing; the
profile contains no fixed ESP32 GPIO assignments or fixed slot selection.
`testing: null` explicitly means no automated connector test is defined.

The image URL reserves the existing documentation naming convention:
`https://docs.envnode.de/assets/images/sd-card-0.1/top.png`. Availability and the
review of the actual image remain separate work. No product-image generation
rules or images are incorporated by this profile.

`org.envnode.driver.sd-spi` API 1 and `storage.sd` are the proposed runtime
contract names. Runtime registration, SPI requirement compatibility and binding
this infrastructure device to the existing storage service remain to implement.
The minimum runtime version 0.8.6 uses the current main baseline; it is not a
claim that main 0.8.6 already activates this module. Confirm the release minimum
when the runtime integration is complete. Missing-driver checks must remain
independent of the version check. Provisioning must continue to allow valid
hardware descriptors even when their runtime driver is unavailable.

Validation: profile identity/name/template consistency and deterministic compact
CBOR encoding with main's existing encoder pass; payload size is 667 bytes
within the generator's 2016-byte limit. Full generator/catalogue validation and
firmware decoder/provisioning tests are pending integration of the unified
profile infrastructure. No EEPROM was written.

Merge acceptance also requires a general bounded diagnostic recording interface:
Raindetector or SCT013 supplies diagnostic records without depending on SPI/FAT
or requiring changes to the SD driver. Naming, rotation, storage limits and
storage failures remain the responsibility of the recording/storage layers.
