# Compact CBOR Profile 0.1 — Candidate

**Status:** Candidate for measurement; not yet an accepted EEPROM format

This profile maps the JSON authoring model to deterministic CBOR. It separates immutable type
description from mutable instance, manufacturing and calibration data. It does not change the
meaning of schema 0.1 and contains no executable code.

## Representation rules

- JSON remains the authoring and interchange representation.
- `$schema` and `assessment` are authoring-only and are not encoded.
- Map keys use globally unique unsigned integers defined by the schema. This avoids ambiguous
  maps when optional fields and future schema revisions are combined.
- Known interoperable vocabulary values may use assigned unsigned integers. Unknown
  manufacturer-controlled IDs remain UTF-8 strings and never require central registration.
- Semantic versions are encoded as arrays of unsigned integers.
- UUIDs are encoded as 16-byte byte strings.
- Deterministic map ordering follows RFC 8949: encoded-key length first, then bytewise order.
- Readers reject missing or invalid required fields. Unknown optional numeric keys are skipped.
- Unknown product type IDs are accepted. Unknown required platform, safety, driver or capability
  contracts remain a compatibility failure rather than a format failure.

The exact key and vocabulary tables currently live in
`tools/measure_descriptor_encoding.py`. They must move into a normative generated specification
before firmware implementation.

## Measured layouts

The initially proposed split layout was measured first:

| Address | Size | Purpose |
| --- | ---: | --- |
| `0x0000–0x06FF` | 1792 B | Static descriptor bank A |
| `0x0700–0x0DFF` | 1792 B | Static descriptor bank B |
| `0x0E00–0x0EFF` | 256 B | Mutable data bank A |
| `0x0F00–0x0FFF` | 256 B | Mutable data bank B |

Each bank includes a 32-byte envelope. The inactive bank is written and verified first; its magic
is committed last. The reader selects the newest valid generation. Interrupted writes therefore
leave the previous bank selectable.

Static descriptor data contains object kind, product type identity, hardware revision,
compatibility contracts, human description and hardware description. Mutable data contains the
instance UUID, manufacturing values and calibration records.

Measured on the current examples:

| Descriptor | Static bank used | Static free | Mutable bank used | Mutable free |
| --- | ---: | ---: | ---: | ---: |
| EnvNode Mini 0.3 | 1711 B | 81 B | 70 B | 186 B |
| DuoRelay 0.3 | 567 B | 1225 B | 70 B | 186 B |
| AnalogHydroPressure 0.3 | 799 B | 993 B | 151 B | 105 B |

Although all examples fit, 81 bytes of static headroom for the board is not sufficient for a
format intended to evolve. The compact representation also makes a simpler layout possible:

| Address | Size | Purpose |
| --- | ---: | --- |
| `0x0000–0x07FF` | 2048 B | Complete descriptor bank A |
| `0x0800–0x0FFF` | 2048 B | Complete descriptor bank B |

| Descriptor | Complete bank used | Free per bank |
| --- | ---: | ---: |
| EnvNode Mini 0.3 | 1742 B | 306 B |
| DuoRelay 0.3 | 598 B | 1450 B |
| AnalogHydroPressure 0.3 | 911 B | 1137 B |

**Current recommendation:** use the two complete 2048-byte banks. It has fewer recovery states,
keeps every valid bank self-contained and gives the Mini materially more useful headroom. The
cost is rewriting the complete descriptor when manufacturing or calibration data changes. Those
are provisioning operations rather than high-frequency runtime writes, so this is currently the
better tradeoff.

## Candidate 32-byte envelope

| Offset | Size | Field |
| ---: | ---: | --- |
| 0 | 4 | Magic (`ENHD`) |
| 4 | 1 | Envelope version (`1`) |
| 5 | 1 | Payload encoding (`1` = compact CBOR profile 0.1) |
| 6 | 1 | Object kind (`0` board, `1` module) |
| 7 | 1 | Record kind (`0` complete descriptor; other values reserved) |
| 8 | 2 | Schema major |
| 10 | 2 | Schema minor |
| 12 | 4 | Generation counter |
| 16 | 2 | Payload length |
| 18 | 2 | Flags, currently zero |
| 20 | 4 | Payload CRC-32 |
| 24 | 4 | Header CRC-32 with this field zeroed |
| 28 | 4 | Reserved, must be written zero and ignored by version-1 readers |

Multi-byte integers use big-endian byte order. Generation comparison must use wrap-safe unsigned
arithmetic. A bank is selectable only when its magic, supported envelope fields, bounds, header
CRC and payload CRC are valid.

## Open points before acceptance

- Freeze the complete numeric key and interoperable vocabulary tables.
- Confirm that manufacturing and calibration writes are sufficiently infrequent to rewrite one
  complete inactive bank per accepted update.
- Define authenticity and user-approval policy; CRC detects corruption but not malicious data.
- Confirm whether documentation text belongs in both redundant descriptor banks or may be
  shortened without violating the self-description requirement.
- Specify migration from legacy ENID/EMID version 1 without overwriting it until a new descriptor
  has been verified.

Measurements are generated with:

```text
python3 tools/measure_descriptor_encoding.py
```
