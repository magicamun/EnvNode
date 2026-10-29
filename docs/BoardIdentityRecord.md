# EnvNode Board Identity Record Specification

**Status:** Version 2 (Version 1 read-compatible)

**Record size:** 32 bytes

**Architecture decision:** [ADR-0014: Board Identity and Provisioning](adr/0014-board-identity-and-provisioning.md)

## 1. Purpose and scope

This document defines the canonical binary `BoardIdentityRecord` stored in an EnvNode board
identity EEPROM. It specifies the byte-level contract shared by normal EnvNode firmware,
provisioning firmware and diagnostic tools. The EEPROM contains physical identity only;
firmware-owned `BoardProfile` definitions remain authoritative for hardware resources as decided
by ADR-0014.

This document does not define an EEPROM driver, provisioning UI, authorization policy or firmware
implementation.

The record starts at user-memory byte address `0x00` of the board identity EEPROM and occupies
addresses `0x00` through `0x1F`. EEPROM addresses outside that range are not part of this record
and must not be modified by a Board Identity writer. EnvNode Mini Revision 0.8 uses a `24LC32`;
Revision 0.2 used a `24AA025E48` whose factory-programmed EUI-48 area was likewise outside this
record.

## 2. Version 2 byte layout

Offsets are zero-based. All multi-byte integers use little-endian byte order, independently of
the processor's native byte order.

| Offset | Size | Field | Encoding | Version 2 meaning |
| ---: | ---: | --- | --- | --- |
| 0 | 4 | Magic | ASCII bytes | `ENID`, exactly `45 4E 49 44` |
| 4 | 1 | `BoardIdentityFormatVersion` | `uint8` | `2` |
| 5 | 1 | Record length | `uint8` | `32` (`0x20`) |
| 6 | 2 | `BoardProfileId` | little-endian `uint16` | Stable numeric profile identifier |
| 8 | 1 | `BoardRevision.major` | `uint8` | Physical PCB revision major component |
| 9 | 1 | `BoardRevision.minor` | `uint8` | Physical PCB revision minor component |
| 10 | 1 | Serial length | `uint8` | Number of serial bytes, 0 through 17 |
| 11 | 17 | `BoardSerialNumber` | ASCII bytes | Serial text followed by zero padding |
| 28 | 4 | CRC | little-endian `uint32` | CRC-32/ISO-HDLC over bytes 0 through 27 |

The total size is exactly 32 bytes. No field alignment or implicit padding exists.

### 2.1 Field rules

`BoardIdentityFormatVersion` versions the binary EEPROM encoding. It is not a firmware version,
board revision or `BoardProfile` version. Version 2 readers accept versions 1 and 2 with record
length `32`; writers always emit version 2.

`BoardProfileId` has a stable 16-bit EEPROM representation. Numeric IDs are assigned in this
specification and, once assigned, shall never be reused for another physical board family. The
assigned profiles are:

| Numeric ID | Encoded bytes | Profile |
| ---: | --- | --- |
| 0 | `00 00` | `EnvNodeMainboard` |
| 1 | `01 00` | `EnvNodeWeather` |
| 2 | `02 00` | `EnvNodeMini` |

The C++ declaration uses `enum class BoardProfileId : uint8_t` with explicit numeric assignments.
That in-memory enum representation is not the EEPROM encoding; source assignments must continue
to match this registry. No IDs are assigned here to EnvNode 868, EnvNode Nano or other
prospective boards.

`BoardRevision` is the physical PCB revision. The two components are unsigned integers in the
range 0 through 255 and are displayed as `major.minor`. They are independent of firmware and
record format versions. Revision `255.255` is reserved and invalid so that an erased revision
cannot become semantically valid. Firmware must explicitly decide which revisions it supports
for each `BoardProfileId`.

A third PCB revision component, written `major.minor.patch`, identifies an electrically
and mechanically unchanged silkscreen/documentation or fabrication-output correction.
Patch numbers start at 1 and increase for subsequent such corrections. For example,
Mini PCB `0.8.1` retains Board Identity revision `0.8`, and Weather PCB `0.2.1` retains
`0.2`. The patch component is not encoded in EEPROM, does not change the board profile,
and does not require reprovisioning. Guides for the corresponding `major.minor`
revision continue to apply. A patch must not change components, pin assignments,
copper, holes, board outline, or mounting/connector geometry. Such changes require
a separate hardware revision and an explicit compatibility assessment. Patch provenance
belongs in the PCB title block, fabrication output and revision notes; historical
source fingerprints and validation records remain tied to their original source state.

`BoardSerialNumber` contains at most 17 ASCII characters. Letters `A-Z`/`a-z`, digits, hyphen,
underscore and dot are permitted. Serial matching is case-sensitive. Length 0 means unassigned;
an otherwise valid record may still select its profile, but reports `UnassignedSerial`.

Version 1 stored an unsigned 32-bit numeric serial at offsets 10 through 13 and reserved offsets
14 through 27. Readers convert a non-zero legacy value to decimal text; zero becomes unassigned.
Rewriting such an identity migrates it to version 2.

Version 1 writers set all reserved bytes to zero and include them in the CRC. Readers validate
integrity before interpreting fields, but otherwise ignore reserved-byte values. In particular,
non-zero reserved bytes do not invalidate a version 1 record. Readers must not assign semantics to
them without a future specification.

## 3. CRC-32

The integrity field uses **CRC-32/ISO-HDLC**, also known as CRC-32/ADCCP and the common Ethernet,
ZIP and `crc32` variant. Its complete parameters are:

| Parameter | Value |
| --- | --- |
| Width | 32 bits |
| Polynomial | `0x04C11DB7` |
| Initial value | `0xFFFFFFFF` |
| Input reflected (`refin`) | true |
| Output reflected (`refout`) | true |
| Final XOR (`xorout`) | `0xFFFFFFFF` |
| Check value for ASCII `123456789` | `0xCBF43926` |

The calculation consumes the 28 bytes at offsets 0 through 27 in ascending offset order. The CRC
field at offsets 28 through 31 is excluded. The resulting 32-bit value is stored least-significant
byte first.

## 4. Reading and validation

A reader should produce a structured status rather than a single valid/invalid flag. It should
apply these checks in order so blank storage and compatibility failures remain distinguishable:

1. Read all 32 bytes from the expected storage location.
2. If the read fails or the device is absent, report `StorageUnavailable`.
3. If all 32 bytes are `0xFF`, report `NotProvisioned`. A reader may additionally treat all
   `0x00` as `NotProvisioned` for devices or development tools that initialize storage to zero.
4. If bytes 0 through 3 are not `ENID`, report `InvalidMagic`.
5. If the format version is not supported, report `UnsupportedFormat`.
6. If the length is not exactly 32 for formats 1 or 2, report `InvalidLength`.
7. Calculate and compare the CRC; a mismatch is `InvalidCRC`.
8. Decode the `BoardProfileId`; a syntactically valid, integrity-valid record whose ID is not in
   the running firmware is `UnknownBoardProfile`.
9. Validate the revision against the selected firmware profile. Reserved `255.255` is
   `InvalidRevision`; a well-formed revision that the profile does not support is
   `UnsupportedRevision`.
10. Validate serial length and characters; invalid content is `InvalidSerial`.
11. If the serial length is zero, report the non-fatal diagnostic `UnassignedSerial`.
12. Otherwise report `Valid`.

Only `Valid` and the otherwise-valid `UnassignedSerial` state provide a usable board identity and
may select the record's `BoardProfileId`. `NotProvisioned`, `StorageUnavailable`, `InvalidMagic`,
`UnsupportedFormat`, `InvalidLength` and `InvalidCRC` do not provide an EEPROM profile; ADR-0014's
explicit development build fallback policy may apply. `UnknownBoardProfile`, `InvalidRevision`
and `UnsupportedRevision` must not fall back: they indicate an unsupported hardware/firmware
combination, and board-dependent normal runtime hardware must not start.

Blank detection is performed before magic or CRC validation. An erased EEPROM is therefore
`NotProvisioned`, not a corrupt record merely because its CRC is invalid. Any partially erased or
otherwise mixed content continues through normal validation and is classified by the first failed
check.

## 5. Encoding and persistence

The record is a byte protocol. Implementations must encode and decode each field explicitly at the
specified offsets. They must not persist a C++ struct by casting or dumping its memory: padding,
alignment, enum layout, compiler behavior and native endianness are not part of this format.

For a provisioning write:

1. Validate the requested profile ID, revision and serial policy.
2. Construct all 32 bytes in RAM, initializing reserved bytes to zero.
3. Encode every field explicitly and calculate the CRC over bytes 0 through 27.
4. Invalidate the stored magic first, then write bytes 4 through 31, and write the four magic bytes
   last where the EEPROM and driver permit this ordering. This reduces the chance that an
   interrupted write appears valid; it does not provide transactional atomicity.
5. Read back the complete 32-byte record through the normal read path.
6. Decode and validate the read-back, and compare all identity fields with the requested values.
7. Report success only after verification. A successful write takes effect after reboot and must
   not change the active profile in the current boot.

EEPROM page boundaries, write-cycle completion and retry behavior are storage-driver concerns.
Where stronger power-loss guarantees are required, the storage design must add an independently
specified redundant-slot or transactional scheme; it must not change this record layout silently.

## 6. Complete example

This example describes an EnvNode Mini PCB revision 0.8 with serial `MINI-A01`.

| Field | Value | Encoded bytes |
| --- | --- | --- |
| Magic | `ENID` | `45 4E 49 44` |
| Format version | 2 | `02` |
| Record length | 32 | `20` |
| `BoardProfileId` | 2 (`EnvNodeMini`) | `02 00` |
| Revision | 0.8 | `00 08` |
| Serial length | 8 | `08` |
| Serial | `MINI-A01` | `4D 49 4E 49 2D 41 30 31`, then `00` × 9 |
| CRC-32/ISO-HDLC | Calculated over bytes 0–27 | little-endian |

CRC input, bytes 0 through 27:

```text
45 4E 49 44 02 20 02 00 00 08 08 4D 49 4E
49 2D 41 30 31 00 00 00 00 00 00 00 00 00
```

The final four bytes contain the CRC calculated for that input.

## 7. Format evolution

- Existing format versions must never be reinterpreted incompatibly.
- An incompatible layout or interpretation requires a new format version.
- Record length is part of each version's contract; versions 1 and 2 are 32 bytes.
- `BoardProfileId` assignments remain stable independently of record format versions.
- Version 2 writers zero-pad unused serial bytes.
- Readers reject unsupported format versions instead of guessing a compatible layout.

## 8. Self-contained port map (ENPM 1)

ENID remains unchanged at address 0. New board provisioning additionally writes
ENPM before updating ENID. ENPM banks occupy `0x0100..0x07ff` and
`0x0800..0x0eff`; bytes `0x0020..0x00ff` and `0x0f00..0x0fff` are preserved.
Module ENHD storage is unaffected.

The 24-byte little-endian header is:

| Offset | Bytes | Meaning |
| --- | --- | --- |
| 0 | 4 | `ENPM`, committed last |
| 4 | 1 | Format version 1 |
| 5 | 1 | Header length 24 |
| 6 | 2 | Payload length, 1..1024 |
| 8 | 4 | Generation, wraps modulo 2^32 |
| 12 | 4 | Payload CRC32 |
| 16 | 4 | CRC32 of header bytes 0..15 |
| 20 | 4 | Reserved, zero |

Payload uses definite-length canonical CBOR:

```text
[1, familyId, [revisionMajor, revisionMinor], name,
 [minimumFwMajor, minimumFwMinor, minimumFwPatch], serial,
 [[gpio, capabilityMask], ...],
 [[bus, sda, scl], ...],
 [[slot, identityAddress, aux1, aux2, chipSelect, fiveVolt], ...],
 [spiClock, spiMiso, spiMosi]]
```

Family IDs 0..254 are stable catalog assignments; 255 is reserved. The decoder
limits GPIOs to 32 entries, I2C buses to two and slots to two. Capability-mask bits
0..4 are digital input/output, interrupt, internal pull-up and analog input.
Names are at most 95 bytes, serials retain the ENID limit, and firmware version
components are unsigned 16-bit integers. Pin numbers 0..254 and nonzero 8-bit
capability masks are stored without assuming current ESP32 support; runtime
rejects unsupported pins/bits. I2C bus/slot IDs are 0/1; module identity
addresses remain 0x52/0x53. The serial in published templates is empty and is
filled by provisioning.

The generic codec checks structure and bounds. Runtime separately checks the
ESP32 pin restrictions, fixed I2C0 bootstrap pins 21/22, conflicting bus/slot
bindings, and minimum firmware. A validated map can therefore be provisioned
without the current firmware being able to operate it.

A map-aware ENID reader permits unregistered family/revision values only when a
valid ENPM record matches family, revision and serial. The original strict ENID
API and legacy ENID-only boot path remain available. A present corrupt, unknown,
unreadable or mismatched map blocks hardware initialization; it never silently
selects a compiled profile. Older firmware retains its original strict ENID
validation and rejects an unknown family/revision.

See [ADR 0015](adr/0015-self-contained-board-port-map.md) for failure handling.
