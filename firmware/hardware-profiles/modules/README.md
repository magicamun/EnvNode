# SD-Card profile integration

EnvNode-owned content: see repository LICENSE
(LicenseRef-EnvNode-Use-and-Redistribution-1.0).

`sd-card-0.1.json` follows the unified profile format alongside DuoRelay 0.3.
Its compact CBOR template and catalogue entry remain available for hardware
identification and testboard provisioning. The classic ESP32 firmware does not
operate SD: compatibility reports MissingDriver and inventory UnsupportedDriver.
Descriptor provisioning is independent of runtime support; no firmware auto-writes
the EEPROM. SD runtime code remains in Git history before its removal.

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

The minimum-version field alone does not promise runtime support. Generic
resource ownership remains visible for physically inserted unsupported modules,
so GPIOs are not silently reused underneath connected hardware. The shared
application core and independent EnvNodeDiagnostics producer interface remain;
a future S3 profile can add a storage transport without changing event producers.

Historical physical SD validation on 2026-11-03 is documented under
`docs/SdCardSpiBringup.md`. Current UDP setup and receiver-side validation are in
`docs/DiagnosticRecording.md` and `docs/UdpLoggingValidation.md`. RainDetector
already records every completed gate; SCT013 integration remains separate work.
