# EnvNode Hardware Descriptor Schema 0.1 Evaluation

**Status:** Exploratory model; not an EEPROM format decision
**Scope:** EnvNode Mini 0.3, DuoRelay 0.3 and AnalogHydroPressure 0.3

## Artifacts

- [`descriptor-schema-0.1.json`](descriptor-schema-0.1.json) defines the common authoring model.
- [`envnode-mini-0.3.json`](examples/envnode-mini-0.3.json) describes the current two-slot board.
- [`duo-relay-0.3.json`](examples/duo-relay-0.3.json) describes two GPIO-controlled relays.
- [`analog-hydro-pressure-0.3.json`](examples/analog-hydro-pressure-0.3.json) describes the ADS1115-based 4–20 mA input.

The examples deliberately contain an `assessment` member. It records evidence, assumptions and
open questions next to each draft. It is authoring metadata and is excluded from the proposed
EEPROM payload. `$schema` is likewise excluded from the EEPROM payload because `schemaVersion`
identifies the payload schema.

## Schema 0.1 model

The common root separates:

1. manufacturer-controlled type and instance identity;
2. compatibility contracts;
3. human-readable description;
4. board-provided resources or module resource requirements and devices;
5. manufacturing data;
6. calibration records; and
7. authoring-only evidence and uncertainty annotations.

Unknown product type IDs are not rejected merely for being unknown. Compatibility is determined
from the platform and safety contracts, available drivers and capabilities, logical resource
resolution and later occupancy validation.

Module descriptors contain only logical connector resources such as `I2C0`, `AUX_GPIO1` and
`+5V`. Only a board descriptor contains platform bindings such as `gpio:4`. This establishes the
boundary required to resolve the same module against different compatible boards.

Contract and capability identifiers in schema 0.1 are namespaced strings. Their current names are
proposals, not an established registry. Manufacturer-controlled product `typeId` values do not
require central allocation; interoperability contracts still require stable public definitions.

## Size measurement

All sizes are byte counts using UTF-8. `Pretty JSON` is the checked-in authoring file including
`$schema` and `assessment`. `Compact payload JSON` removes those two authoring-only members and
uses JSON without insignificant whitespace. `Deterministic CBOR` encodes that same payload using
RFC 8949 deterministic map-key ordering, ordinary text keys and no application-specific string
dictionary or numeric field tags.

| Descriptor | Pretty JSON | Compact payload JSON | Deterministic CBOR | CBOR reduction from compact JSON |
| --- | ---: | ---: | ---: | ---: |
| EnvNode Mini 0.3 | 7,196 B | 4,415 B | 3,626 B | 17.9% |
| DuoRelay 0.3 | 3,419 B | 1,681 B | 1,415 B | 15.8% |
| AnalogHydroPressure 0.3 | 4,505 B | 1,895 B | 1,618 B | 14.6% |

The measurements describe the draft model, not final records. Envelope bytes, bank metadata,
alignment, manufacturing allocation and calibration growth are not included.

## Consequences for the 4 KiB EEPROM

A single CBOR copy of every example fits in 4 KiB, although the Mini leaves only 470 bytes before
adding an envelope or separately allocated records. Two complete copies do not fit for EnvNode
Mini: two current CBOR payloads alone require 7,252 bytes before envelopes. DuoRelay would require
2,830 bytes and AnalogHydroPressure 3,236 bytes before headers and reserved areas.

Therefore schema 0.1 does not justify adopting two equal full-descriptor banks yet. At least one
of the following must be evaluated first:

- a more compact schema-driven binary mapping using numeric field keys and enumerated common
  contracts;
- removal of derivable or repeated board resource data without losing self-description;
- asymmetric or variable-size banks;
- atomic records or sections rather than complete-image banks; or
- a larger EEPROM for boards whose complete self-description exceeds the dual-bank budget.

Compact JSON already exceeds 4 KiB for the current board example. Plain CBOR
is viable for one copy but its savings are modest because repeated field and contract names remain
text strings. A schema-driven compact CBOR profile should be measured before choosing a custom
binary format.

## Explicit assumptions and unresolved facts

All per-descriptor assumptions are recorded in each file. The most consequential common ones are:

- namespaced product, platform, safety, driver, capability and calibration IDs are proposed;
- the all-zero instance UUIDs are placeholders for manufacturing-provisioned values;
- firmware version `0.5.0` is the current version, not a verified minimum;
- documentation URLs using `example.invalid` are deliberate placeholders;
- no qualified board rail-current budgets are available;
- trust, signature and third-party descriptor approval policy remains open;
- manufacturing values and actual calibration coefficients are unavailable.

For AnalogHydroPressure specifically, address `0x48` and ADS1115 channel 0 are example assumptions.
The board provides address jumpers for `0x48` through `0x4B`, but the assembled jumper selection
has not been established. ADS1115 gain, data rate, input topology, probe range and real calibration
remain open.

For EnvNode Mini, the descriptor intentionally models resources needed to demonstrate discovery
and module binding. It is not yet an agreed exhaustive description of every externally available
ESP32 pin. GPIO capability claims must ultimately be bounded by an approved platform safety
profile.

## Decisions still required

Schema 0.1 is sufficient to compare representations, but not yet to freeze an EEPROM format. The
next decisions are:

1. canonical vocabulary and versioning for platform, safety, driver and capability contracts;
2. whether the board descriptor must enumerate every physical binding or may reference a signed,
   firmware-known platform safety profile for invariant details;
3. compact CBOR key mapping and forward-compatible unknown-field handling;
4. allocation and atomicity for descriptor, manufacturing and calibration data;
5. authenticity and user-approval policy; and
6. representation of hardware assembly options such as the ADS1115 address jumper.

## Follow-up compact-profile measurement

[`CompactCborProfile01.md`](CompactCborProfile01.md) evaluates a schema-driven deterministic CBOR
mapping with numeric field keys, compact known vocabulary and binary UUIDs. With a 32-byte
envelope, the complete examples measure 1742 bytes for EnvNode Mini, 598 bytes for DuoRelay and
911 bytes for AnalogHydroPressure. This makes two complete 2048-byte banks viable in the 4 KiB
EEPROM and is currently preferred over separately banked static and mutable records.
