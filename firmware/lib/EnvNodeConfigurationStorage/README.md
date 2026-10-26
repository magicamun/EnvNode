# EnvNodeConfigurationStorage

EnvNode-owned materials follow the repository LICENSE
(LicenseRef-EnvNode-Use-and-Redistribution-1.0).

Shared Preferences storage and device/network/time/locale persistence. It requires
EnvNodeBaseConfiguration and Arduino Preferences, without EnvNode's complete
Configuration, backup codec, sensor/actuator/controller models, MQTT, HTTP or runtime.

ConfigurationStorage receives a fixed application-owned layout: legacy namespace,
two snapshot-bank namespaces and a metadata namespace. The layout/strings must
outlive the store. `active` selects bank 0/1 or falls back to the legacy namespace;
`cfg_snapshot` contains opaque application bytes. Replacement clears the inactive
bank, writes and verifies exact bytes, then commits the active-bank metadata and
reopens that bank. The application owns record encoding/validation and memory
activation. Clear erases all three data namespaces and retains the active pointer.

BaseSettingsService borrows existing device/network/time/locale structs and fixed
application defaults; it can implement IBaseSettings and INetworkSettings without
a larger application model. Explicit initialization, legacy override loading and
validation/default recovery are separate operations so applications can first
load their own snapshot baseline. Missing hostname uses the legacy device name
unless a snapshot exists. Explicit empty strings override restored values.

Existing persistence behavior is preserved, including sequential partial writes
for ordinary network edits, memory updates only on success, and the pre-existing
post-snapshot-commit reopen semantics. This extraction does not add a transaction
around individual key edits or change power-failure guarantees. No physical NVS
migration/erase is performed merely by introducing the library.

EnvNode's adapter injects weather/weather0/weather1/weather_meta and its existing
WeatherStation/time/locale defaults. EnvNode ConfigurationService retains full
snapshot/backup codecs, restore validation, MQTT, measurements, slots, values and
display handling. Existing public API and NVS keys/formats remain unchanged. The
future test application supplies its own fixed layout and base data structs.


Layouts may additionally specify a dedicated snapshot partition. Without that
optional label, existing single-partition behavior is unchanged. With a label,
startup reads its committed metadata if present; otherwise it keeps the original
NVS data authoritative. Snapshot replacement targets the dedicated partition and
switches only after verified bytes/metadata commit. Ordinary edits use whichever
partition is authoritative. Clear removes configuration data in both partitions.
A missing dedicated partition permits legacy operation but blocks replacement.
EnvNode selects cfg_nvs (256 KiB) through its fixed layout; installation requires
the matching partition table by USB. The layout and label strings must remain
alive for the lifetime of the store. The partition-aware native Preferences double
and migration tests cover both old flat keys and old-bank snapshots.

Snapshot replacement accepts an optional ILogger for phase diagnostics. Existing
callers may omit it. The readback buffer uses checked nothrow allocation; if it
cannot be allocated, metadata are not committed. EnvNode injects its logger into
ConfigurationService so device logs distinguish validation, encoding, write,
verification, commit and activation without printing any configuration contents.
