# EnvNode Descriptor CBOR Keys and Vocabulary 0.1

**Status:** Normative for descriptor schema 0.1

This document assigns stable unsigned integers to fields and shared interoperability vocabulary
used by the compact deterministic CBOR representation. Published numbers are never reused or
assigned a different meaning. New schema revisions may add numbers.

Manufacturer names, product type IDs, instance-local device IDs, logical resource names and
unknown extension vocabulary remain UTF-8 strings. They do not require central registration.

## Field keys

Field keys are globally unique within schema 0.1.

| Code | JSON field | Code | JSON field |
| ---: | --- | ---: | --- |
| 0 | `schemaVersion` | 23 | `summary` |
| 1 | `objectKind` | 24 | `documentationUrl` |
| 2 | `identity` | 25 | `resources` |
| 3 | `compatibility` | 26 | `slots` |
| 4 | `description` | 27 | `kind` |
| 5 | `hardware` | 28 | `platformBinding` |
| 6 | `manufacturing` | 29 | `voltageMillivolts` |
| 7 | `calibration` | 30 | `properties` |
| 8 | `typeId` | 31 | `interface` |
| 9 | `instanceId` | 32 | `identityAddress` |
| 10 | `manufacturer` | 33 | `bindings` |
| 11 | `hardwareRevision` | 34 | `requirements` |
| 12 | `legacyProfileId` | 35 | `devices` |
| 13 | `major` | 36 | `resource` |
| 14 | `minor` | 37 | `driver` |
| 15 | `minimumFirmwareVersion` | 38 | `parameters` |
| 16 | `platform` | 39 | `measurements` |
| 17 | `safetyProfile` | 40 | `serialNumber` |
| 18 | `drivers` | 41 | `productionBatch` |
| 19 | `capabilities` | 42 | `productionDate` |
| 20 | `id` | 43 | `target` |
| 21 | `apiVersion` | 44 | `schema` |
| 22 | `name` | 45 | `values` |

Unknown numeric keys are skipped only where the containing schema object permits future optional
members. Missing or malformed required keys invalidate the descriptor.

## Object and hardware kinds

Codes are interpreted in their field context.

| Context | Code | Meaning |
| --- | ---: | --- |
| `objectKind` | 0 | board |
| `objectKind` | 1 | module |
| resource `kind` | 0 | GPIO |
| resource `kind` | 1 | I²C |
| resource `kind` | 2 | SPI |
| resource `kind` | 3 | power |
| device `kind` | 0 | sensor |
| device `kind` | 1 | actuator |
| device `kind` | 2 | infrastructure |

## Known contract IDs

A contract ID may be encoded as the listed integer or as its full text ID. Unknown contract IDs
must use text. An unknown contract is syntactically valid but is incompatible when required.

| Context | Code | Text ID |
| --- | ---: | --- |
| platform | 1 | `org.envnode.platform.esp32` |
| platform | 2 | `org.envnode.platform.any` |
| safety profile | 1 | `org.envnode.safety.esp32-envnode-mini` |
| safety profile | 2 | `org.envnode.safety.module-interface` |
| interface | 1 | `org.envnode.interface.module-2x7` |
| driver | 1 | `org.envnode.driver.gpio-on-off` |
| driver | 2 | `org.envnode.driver.ads1115` |

## Known capabilities

Unknown capabilities must use their namespaced text representation. Required unknown
capabilities cause incompatibility, not descriptor-format rejection.

| Code | Text value |
| ---: | --- |
| 1 | `digital-input` |
| 2 | `digital-output` |
| 3 | `analog-input` |
| 4 | `supply` |
| 5 | `actuator.on-off` |
| 6 | `sensor.pressure` |
| 7 | `sensor.current` |

## Scalar representation

- semantic versions: CBOR array of unsigned `[major, minor, patch]`;
- hardware revisions: map using keys 13 and 14;
- UUID: 16-byte CBOR byte string;
- dates and free-form identifiers: UTF-8 CBOR text strings;
- absent manufacturing values: CBOR `null`;
- numeric extension properties: the narrowest exact CBOR integer, or IEEE-754 binary64 when a
  non-integral value is required.

Maps use RFC 8949 deterministic ordering. Indefinite-length CBOR items are not allowed.
