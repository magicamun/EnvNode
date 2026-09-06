# EnvNode Board Identity Record Specification

**Status:** Version 1

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
and must not be modified by a Board Identity writer. EnvNode Mini Revision 0.3 uses a `24LC32`;
Revision 0.2 used a `24AA025E48` whose factory-programmed EUI-48 area was likewise outside this
record.

## 2. Version 1 byte layout

Offsets are zero-based. All multi-byte integers use little-endian byte order, independently of
the processor's native byte order.

| Offset | Size | Field | Encoding | Version 1 meaning |
| ---: | ---: | --- | --- | --- |
| 0 | 4 | Magic | ASCII bytes | `ENID`, exactly `45 4E 49 44` |
| 4 | 1 | `BoardIdentityFormatVersion` | `uint8` | `1` |
| 5 | 1 | Record length | `uint8` | `32` (`0x20`) |
| 6 | 2 | `BoardProfileId` | little-endian `uint16` | Stable numeric profile identifier |
| 8 | 1 | `BoardRevision.major` | `uint8` | Physical PCB revision major component |
| 9 | 1 | `BoardRevision.minor` | `uint8` | Physical PCB revision minor component |
| 10 | 4 | `BoardSerialNumber` | little-endian `uint32` | Raw numeric board serial number |
| 14 | 14 | Reserved | bytes | Writers set every byte to `0x00` |
| 28 | 4 | CRC | little-endian `uint32` | CRC-32/ISO-HDLC over bytes 0 through 27 |

The total size is exactly 32 bytes. No field alignment or implicit padding exists.

### 2.1 Field rules

`BoardIdentityFormatVersion` versions the binary EEPROM encoding. It is not a firmware version,
board revision or `BoardProfile` version. A version 1 reader accepts only value `1` and record
length `32`.

`BoardProfileId` has a stable 16-bit EEPROM representation. Numeric IDs are assigned in this
specification and, once assigned, shall never be reused for another physical board family. The
current repository defines only one profile:

| Numeric ID | Encoded bytes | Profile |
| ---: | --- | --- |
| 0 | `00 00` | `EnvNodeMainboard` |

The current C++ declaration uses `enum class BoardProfileId : uint8_t` and gives
`EnvNodeMainboard` the implicit value 0. That in-memory enum representation is not the EEPROM
encoding. Future source declarations should use explicit numeric assignments matching this
registry. No IDs are assigned here to EnvNode 868, EnvNode Nano or other prospective boards.

`BoardRevision` is the physical PCB revision. The two components are unsigned integers in the
range 0 through 255 and are displayed as `major.minor`. They are independent of firmware and
record format versions. Revision `255.255` is reserved and invalid so that an erased revision
cannot become semantically valid. Firmware must explicitly decide which revisions it supports
for each `BoardProfileId`.

`BoardSerialNumber` is the raw numeric value in the range 0 through 4,294,967,295. Value 0 means
unassigned; assigned serial numbers therefore range from 1 through 4,294,967,295. Presentation
formats such as `EN-000012` are not persisted. An otherwise valid record with serial 0 may still
select its board profile, but must report `UnassignedSerial` and is not fully provisioned for
manufacturing purposes.

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
6. If the length is not exactly 32 for format 1, report `InvalidLength`.
7. Calculate and compare the CRC; a mismatch is `InvalidCRC`.
8. Decode the `BoardProfileId`; a syntactically valid, integrity-valid record whose ID is not in
   the running firmware is `UnknownBoardProfile`.
9. Validate the revision against the selected firmware profile. Reserved `255.255` is
   `InvalidRevision`; a well-formed revision that the profile does not support is
   `UnsupportedRevision`.
10. If the serial number is 0, report the non-fatal diagnostic `UnassignedSerial`.
11. Otherwise report `Valid`.

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

This example identifies the current EnvNode Mainboard PCB revision 0.2 with serial number 12. The
repository's KiCad schematic and PCB both declare revision 0.2, and the hardware specification
states that the EEPROM is present starting with that revision.

| Field | Value | Encoded bytes |
| --- | --- | --- |
| Magic | `ENID` | `45 4E 49 44` |
| Format version | 1 | `01` |
| Record length | 32 | `20` |
| `BoardProfileId` | 0 (`EnvNodeMainboard`) | `00 00` |
| Revision | 0.2 | `00 02` |
| Serial | 12 | `0C 00 00 00` |
| Reserved | fourteen zero bytes | `00` × 14 |
| CRC-32/ISO-HDLC | `0xEBE632D6` | `D6 32 E6 EB` |

CRC input, bytes 0 through 27:

```text
45 4E 49 44 01 20 00 00 00 02 0C 00 00 00
00 00 00 00 00 00 00 00 00 00 00 00 00 00
```

Complete 32-byte record:

```text
45 4E 49 44 01 20 00 00 00 02 0C 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 00 D6 32 E6 EB
```

## 7. Format evolution

- Existing format versions must never be reinterpreted incompatibly.
- An incompatible layout or interpretation requires a new format version.
- Record length is part of each version's contract; version 1 is always 32 bytes.
- `BoardProfileId` assignments remain stable independently of record format versions.
- Version 1 writers keep reserved bytes zero. A future compatible use of reserved bytes must not
  change the meaning of existing fields or require version 1 readers to interpret them.
- Readers reject unsupported format versions instead of guessing a compatible layout.
