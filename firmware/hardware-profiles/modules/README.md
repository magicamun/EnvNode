# SD-Card profile integration

EnvNode-owned content: see repository LICENSE
(LicenseRef-EnvNode-Use-and-Redistribution-1.0).

`sd-card-0.1.json` follows the unified profile format alongside DuoRelay 0.3.
Its 657-byte compact CBOR template and catalogue entry are published through
main's regular documentation workflow. The integrated main firmware can provision
and operate it. Earlier builds without an SD runtime driver can still provision
the valid descriptor independently of runtime support. No firmware auto-writes the EEPROM.

The profile describes PCB revision 0.1: SPI, slot-specific SPI_CS, AUX_GPIO1 card
detect (HIGH means inserted) and +5V. The driver contract is
`org.envnode.driver.sd-spi` API 1; its infrastructure device advertises
`storage.sd`. The `activeLevel` parameter applies to card detect. The 24LC32 uses
the interface's existing slot-dependent identity addresses. No fixed ESP32 GPIOs
or fixed slot are embedded in the profile. `testing: null` means no automated
connector test is defined.

The image URL follows the existing documentation convention:
`https://docs.envnode.de/assets/images/sd-card-0.1/top.png`. Actual image review
remains separate; no product-image rules or images are incorporated here.

In the integrated firmware, boot discovery checks compatibility and the device's bus,
chip-select and card-detect bindings before activating access in the discovered
slot. Two SD modules or an unsupported SD contract disable access. System-log
recording stays an explicit persisted user preference. Without an identified SD
module, the manually configured breakout mode remains available. Automatic slot
selection is runtime state, not an EEPROM rewrite or a new backup setting.

The profile minimum runtime version is 0.8.6. Earlier builds of that version
without the SD driver remain blocked by the independent driver-support check;
the version number alone is not a claim of SD support.

Provisioning is tested through the generated catalogue, browser upload endpoint,
instance assignment and EEPROM readback. SD runtime tests cover both slots,
duplicate modules, wrong polarity, incompatible bindings/API and manual fallback.
The physical activation test with the assembled module remains required.

Merge acceptance also requires a general bounded diagnostic recording interface:
Raindetector or SCT013 supplies diagnostic records without depending on SPI/FAT
or requiring changes to the SD driver. Naming, rotation, storage limits and
storage failures remain the responsibility of the recording/storage layers.

The general recording boundary is now implemented in `EnvNodeDiagnostics` and
exercised by the independent system diagnostic producer. See
`docs/DiagnosticRecording.md` for the producer API, per-entity opt-in and the
pending physical recording acceptance. Detailed Raindetector events remain
producer-side work after integration; they do not require SD-driver changes.
