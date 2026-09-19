# ADR-0015: Module Identity Record

## Status

Accepted as the legacy EMID version 1 baseline. The self-describing descriptor schema 0.1
supersedes the profile-registry requirement as the target model.

## Date

2026-09-07

## Context

EnvNode module slots reserve I2C0 addresses `0x52` and `0x53` for optional module-identification
EEPROMs. The firmware needs to distinguish known modules, unknown future modules, blank slots and
invalid records without inferring hardware from other I2C devices or automatically activating an
unsafe driver.

Module identity is manufacturing data. It is separate from the user configuration in NVS and
from the firmware-owned module profile that describes resources, capabilities and future driver
integration.

## Decision

Each provisioned module EEPROM stores the canonical 32-byte record specified in
[`ModuleIdentityRecord.md`](../ModuleIdentityRecord.md). The record contains only a stable module
profile ID, physical revision, serial number, format metadata and an integrity check. It does not
contain pin mappings, driver names, configuration defaults or executable behavior.

The dependency direction is:

```text
slot EEPROM
  -> ModuleIdentity
  -> ModuleProfileId
  -> firmware-owned module profile
  -> offered capabilities and compatible runtime configuration
```

A valid record may identify a module, but identity detection alone must not start a driver or
claim hardware resources. Runtime activation remains an explicit later decision after board,
slot, module-profile and resource compatibility validation.

Unknown profile IDs and unsupported revisions remain visible diagnostic states. They must not be
coerced to a known module. A missing, blank or invalid record leaves the slot in generic mode as
defined by the module-interface specification.

The record codec and semantic validation are independent of I2C and slot selection. A
`ModuleIdentityStore` owns the safe write sequence and readback verification. Slot-specific
instances of the shared 24LC32 adapter own EEPROM addresses `0x52` and `0x53`, and
`ModuleDiscoveryService` reads them independently without activating runtime hardware.

A firmware-owned `ModuleProfileRegistry` resolves supported legacy identity and revision
combinations to typed capability and connector-resource metadata. It is an optional catalog for
EMID version 1 and known products, not a registration requirement for future descriptor-based
modules. These profiles may constrain later configuration choices, but are not executable
configuration and do not activate a runtime component.

## Consequences

- Board and module records cannot be confused because they use different magic values.
- Provisioning tools and normal firmware share one byte-level contract.
- New EMID version 1 module IDs and supported revisions require reviewed firmware changes;
  descriptor-based modules are instead validated through supported contracts and resources.
- Module discovery and module profiles remain separate from future automatic UI suggestions and
  runtime activation.
