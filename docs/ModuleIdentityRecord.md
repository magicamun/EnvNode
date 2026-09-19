# EnvNode Module Identity Record Specification

**Status:** Legacy Version 1; retained for current hardware provisioning and migration
**Record size:** 32 bytes
**Architecture decision:** [ADR-0015: Module Identity Record](adr/0015-module-identity-record.md)

## Purpose

This document defines the canonical record stored at byte address `0x0000` of an optional EnvNode
module-identification EEPROM. Module EEPROMs use I2C0 address `0x52` in Slot A and `0x53` in Slot B.
The record identifies physical hardware only; firmware-owned module profiles remain authoritative
for resources, capabilities, drivers and UI behavior.

Version 1 is the compact legacy format used to provision and validate the current Revision 0.3
hardware. It is intentionally retained as a readable migration source, but it is not the final
self-describing module format. In particular, its numeric `ModuleProfileId` requires firmware to
know the product in advance. Future descriptor records may identify previously unknown products
through declarative driver, capability and logical-resource requirements instead.

## Binary layout

| Offset | Size | Field | Encoding |
| ---: | ---: | --- | --- |
| 0 | 4 | Magic | ASCII `EMID` (`45 4D 49 44`) |
| 4 | 1 | Format version | `1` |
| 5 | 1 | Record length | `32` (`0x20`) |
| 6 | 2 | `ModuleProfileId` | little-endian `uint16` |
| 8 | 1 | Revision major | `uint8` |
| 9 | 1 | Revision minor | `uint8` |
| 10 | 4 | Serial number | little-endian `uint32` |
| 14 | 14 | Reserved | writers set to zero; readers ignore |
| 28 | 4 | CRC-32/ISO-HDLC | little-endian CRC over bytes 0–27 |

The current stable profile assignments are:

| Numeric ID | Module profile | Supported revision |
| ---: | --- | --- |
| 1 | `DuoRelay` | 0.3 |
| 2 | `AnalogHydroPressure` | 0.3 |

Assigned numeric IDs must never be reused for another module family. Serial number zero means
explicitly unassigned; it does not make an otherwise supported identity unusable.

## Validation order and states

Readers validate in this order:

1. transport and exact 32-byte read;
2. all-`FF` or all-zero blank detection;
3. magic;
4. format version;
5. encoded record length;
6. CRC;
7. known module profile ID;
8. valid and supported physical revision;
9. assigned or unassigned serial number.

The resulting states distinguish `StorageUnavailable`, `NotProvisioned`, `InvalidMagic`,
`UnsupportedFormat`, `InvalidLength`, `InvalidCRC`, `UnknownModuleProfile`, `InvalidRevision`,
`UnsupportedRevision`, `UnassignedSerial` and `Valid`.

Only `Valid` and `UnassignedSerial` identify a supported module. Every other state leaves the slot
in generic mode. Detection by itself never activates module hardware.

After successful identity validation, firmware may resolve the identity against its immutable
`ModuleProfileRegistry`. The current DuoRelay profile advertises two on/off outputs and requires
both slot-specific auxiliary GPIOs plus the 5 V supply. AnalogHydroPressure advertises an analog
current input and local probe supply and requires I2C0 plus the 5 V supply. The identity EEPROM
itself is not counted as a functional module resource.

## Write procedure

Writers validate and encode the complete record first. They then invalidate bytes 0–3, write bytes
4–31, write the final magic bytes last, read back all 32 bytes, validate them and compare both the
encoded bytes and decoded identity. A successful write becomes eligible for discovery after the
next scan or reboot; it does not change already active hardware automatically. A failed or invalid
record in one slot does not prevent discovery of the other slot.

## Complete example

This example identifies a DuoRelay revision 0.3 with serial number 12:

```text
45 4D 49 44 01 20 01 00 00 03 0C 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 00 8D FB B8 98
```

The stored CRC is `0x98B8FB8D`.
