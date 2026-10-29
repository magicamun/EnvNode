# ADR 0015: Self-contained board port map

EnvNode: see repository LICENSE.

Keep legacy ENID at address 0. Add an independently versioned ENPM record in two
1792-byte banks starting at 0x0100 and 0x0800. The first 256 bytes and final 256
bytes remain outside these banks. Module ENHD storage is unchanged.

ENPM contains board family ID, hardware revision, serial number, display name,
minimum runtime firmware, GPIO capability masks, I2C pin pairs, module slot
bindings/identity addresses, and SPI pins. It contains no drivers, images or test
geometry. It is sufficient to reconstruct runtime resources without a built-in
revision table or network access.

A 24-byte header contains magic ENPM, format version 1, header size 24, payload
length (u16 LE), generation (u32 LE), payload CRC32, header CRC32 over bytes 0..15,
and four zero reserved bytes. Payload is definite-length canonical CBOR. Writes
invalidate the inactive bank's magic, write payload/header and commit magic last;
readback verifies bytes and both CRCs. Generation comparison handles wraparound.

Provisioning writes/verifies ENPM before updating ENID. Both must match family,
revision and serial at boot. Interrupted cross-record updates fail closed;
repeating provisioning restores consistency. Legacy ENID-only devices keep their
existing boot path. A present corrupt/incompatible map is not silently replaced
with a compiled map. The generic ENPM codec/writer never gates provisioning on
runtime support. The ESP32 runtime adapter validates actual usable pins (including the UART0 reservation) and
firmware version before initializing hardware.

The generic format carries byte-sized pin numbers and capability masks; unknown
capability bits or pins outside the current ESP32 range are retained during
provisioning and rejected by the current runtime adapter. Physical port-contract
changes require a new hardware revision/family; a published identity must never
be reused with different bindings. This also prevents older firmware from using
its compiled mapping for a different physical port contract.
