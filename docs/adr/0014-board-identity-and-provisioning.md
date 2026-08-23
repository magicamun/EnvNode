# ADR-0014: Board Identity and Provisioning

## Status

Accepted

## Date

2026-08-23

## Context

EnvNode already models hardware through `BoardProfileId`, `BoardRevision`, `BoardProfile`,
`currentBoardProfile()` and `BoardCapabilities`. The active profile is currently selected by
a build flag. This boundary was intentionally kept narrow so that a hardware identity source
could later select the same firmware-owned profiles without changing their consumers.

Future products such as EnvNode Mainboard, EnvNode 868 and EnvNode Nano need one firmware
architecture that can identify the physical PCB on which it is running. Existing development
boards must remain usable even when they have no identity EEPROM or have not been provisioned.
Provisioning will initially be available through the normal EnvNode Web UI, while a later,
smaller provisioning firmware must be able to reuse the same storage implementation.

Board identity is persistent manufacturing data. It is distinct from application configuration
in NVS and from firmware build identity.

## Decision

### Identity and profile are separate models

`BoardIdentity` answers, "What physical board is this?"

`BoardProfile` answers, "What hardware resources and capabilities does this board provide?"

The dependency direction is:

```text
EEPROM
  -> BoardIdentity
  -> BoardProfileId
  -> BoardProfile
  -> BoardCapabilities
  -> hardware consumers
```

EEPROM contains identity and record-versioning data only. It does not contain pin maps, GPIO
capability tables, I2C or SPI definitions, or other hardware-resource descriptions. Immutable,
typed `BoardProfile` definitions in the firmware remain authoritative. Adding or changing a
board's resources therefore requires a firmware change, review and test rather than an EEPROM
rewrite.

`BoardRevision` is physical identity metadata used when validating that the running firmware
supports the identified board revision. It does not make the EEPROM a resource-description
database. Firmware profile lookup must explicitly support the identified `BoardProfileId` and
revision; it must not infer resources from revision values.

### Identity record

The conceptual EEPROM record contains at least:

- record magic
- record format or schema version
- `BoardProfileId`
- `BoardRevision`
- a stable board serial number or equivalent board serial identity
- an integrity check such as a CRC

This ADR does not define field widths, byte offsets, byte order, EEPROM technology or the exact
integrity algorithm. A separate Board Identity Record Specification will define the binary and
electrical contract so it can evolve independently and be shared by all writers and readers.

The serial number identifies the individual physical board. It is not an MQTT device name,
user-visible installation name or replacement for runtime configuration.

### Storage boundary

A single `BoardIdentityStore` abstraction owns Board Identity persistence. It provides the
shared read, validation-related transport results and write operations needed by both normal
firmware and future provisioning firmware. EEPROM bus and device details remain behind this
boundary.

Record encoding and semantic validation are shared Board Identity infrastructure as well;
normal and dedicated provisioning firmware must not create separate EEPROM formats or driver
implementations.

The normal runtime consumes only a resolved, immutable identity result. Web handlers, Sensors,
MQTT and other application modules do not access the EEPROM directly.

### Boot resolution

Board Identity is resolved exactly once during boot, before any board-dependent hardware is
initialized:

```text
Boot
  -> initialize only minimal identity-access hardware
  -> read and validate BoardIdentity
  -> resolve a supported BoardProfile
  -> freeze the resolution result for this boot
  -> initialize BoardProfile-dependent hardware
  -> start the normal EnvNode runtime
```

Identity access must use only the minimal bootstrap resources that are fixed by the platform's
identity-access contract. It must not depend on a `BoardProfile` that has not yet been selected.
The concrete bootstrap wiring and EEPROM device contract belong in the later record/hardware
specification.

`currentBoardProfile()` and `BoardCapabilities::current()` continue to present one stable active
profile to existing consumers. Their selection boundary changes from build-time-only to the
frozen boot resolution; consumers remain independent of whether identity came from EEPROM or
the development fallback.

A provisioning write never changes the active profile in a running process. The newly written
identity becomes eligible for selection only after a reboot. Live profile replacement is not
supported.

### Transitional source policy

The resolver distinguishes at least these sources:

- `EEPROM`: a valid, supported EEPROM identity selected the profile
- `BuildFallback`: the explicit build-time development profile was used

Resolution follows this deterministic policy:

1. Read and validate the EEPROM record.
2. If its format, integrity, `BoardProfileId` and revision are supported, select the matching
   firmware-owned profile and report source `EEPROM`.
3. If EEPROM is absent, unreadable, blank, or contains a malformed or integrity-invalid record,
   select the explicit build-time development profile and report source `BuildFallback`.
4. If the record is syntactically valid and passes integrity validation but names an unknown
   `BoardProfileId` or an unsupported revision, do not fall back. Report an unsupported identity
   and do not start board-dependent normal runtime hardware.

The distinction in step 4 prevents firmware/hardware incompatibility from being mistaken for an
unprovisioned prototype. The firmware must never guess, coerce an identifier, choose the first
compiled profile or map an unknown board to a similar board.

Fallback is a development compatibility mechanism, not successful EEPROM identification. Every
fallback records the reason and exposes both the `BuildFallback` source and selected build
profile in diagnostics. Consequently, a corrupt record can never *silently* select a profile.
Production policy may later disable build fallback without changing the identity or profile
models.

### Validation and diagnostics

A record is rejected when any of the following applies:

- magic is invalid
- schema or format version is unsupported
- integrity validation fails
- `BoardProfileId` is unknown to the running firmware
- the identified board revision is unsupported by the selected firmware profile

Diagnostics must distinguish storage absence/read failure, blank record, malformed record,
integrity failure, unsupported schema, unknown profile and unsupported revision. They must also
expose, where available and safe to display:

- resolution source
- resolved `BoardProfileId` and `BoardRevision`
- board serial identity
- active firmware profile metadata
- whether reboot is required after provisioning

An unsupported identity is a safe boot failure for the normal board-dependent runtime, not a
reason to initialize guessed hardware. Minimal diagnostics and provisioning may remain available
only where their bootstrap hardware can operate without a resolved profile.

### Provisioning architecture

Initial provisioning may be an administration function in the normal EnvNode Web UI, but it is
an adapter over the shared Board Identity service:

```text
Normal EnvNode Web provisioning
              |
              v
      BoardIdentity service
              |
              v
       BoardIdentityStore
```

Provisioning logic is isolated from Sensors, Measurements, MQTT, Actuators, Controllers and the
normal runtime composition. A write must validate all supplied fields, encode the canonical
record, write through `BoardIdentityStore`, read it back, and verify the persisted record before
reporting success. Successful provisioning reports that reboot is required; it does not mutate
the active boot resolution.

Provisioning is a privileged and potentially hardware-breaking operation. The Web adapter must
require an explicit user action and confirmation and must not provision automatically during
boot or ordinary configuration save. The concrete authorization and physical-presence policy is
deferred to the provisioning design, but the service boundary must permit such policy to be
enforced by its caller.

A future dedicated provisioning firmware reuses the same Board Identity model, profile metadata,
record codec and `BoardIdentityStore`. It may contain only the infrastructure required for EEPROM
access, identity handling, WiFi/network setup, Web UI, diagnostics and provisioning. It does not
need Sensors, Measurements, MQTT, Home Assistant Discovery, Actuators or Controllers.

## Consequences

### Positive

- physical identity and firmware-owned hardware capabilities remain clearly separated
- one firmware architecture supports unprovisioned prototypes and EEPROM-equipped products
- BoardProfile selection is deterministic and immutable for the duration of a boot
- existing hardware consumers continue to depend on `BoardProfile` and `BoardCapabilities`
- unsupported future boards fail safely instead of receiving a guessed pin map
- normal and dedicated provisioning firmware share one storage format and implementation
- diagnostics make fallback and incompatibility visible

### Negative

- early boot needs a bootstrap hardware contract independent of the selected profile
- development fallback can run with corrupt or missing identity and therefore must remain
  conspicuous in diagnostics
- supported identity record versions and board revisions create an explicit firmware
  compatibility matrix
- provisioning adds privileged write behavior and a reboot step
- a dedicated binary record specification and provisioning security design are still required

## Alternatives considered

### Store complete hardware descriptions in EEPROM

Rejected because it would make mutable manufacturing data authoritative for pin safety, duplicate
`BoardProfile`, complicate validation and allow firmware behavior to change without a firmware
review.

### Keep only build-time BoardProfile selection

Rejected because product variants would require separately selected firmware images and could not
reliably identify the physical PCB or its serial identity.

### Fall back for an unknown but valid identity

Rejected because a valid unknown identifier indicates firmware/hardware incompatibility, not an
empty prototype. Selecting the build profile could initialize unsafe resources on the wrong PCB.

### Change BoardProfile immediately after provisioning

Rejected because existing services may already own resources from the old profile. Reboot-only
activation makes the lifecycle deterministic and avoids partial hardware reinitialization.

### Implement EEPROM access separately in each firmware

Rejected because divergent formats, validation and write behavior would make provisioning
unreliable and prevent the normal firmware from verifying records written by the dedicated tool.
