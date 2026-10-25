# Configuration backup and restore

EnvNode application configuration is backed up as versioned JSON. Board identity and module
EEPROM descriptors remain manufacturing data and are never exported or restored here.

`ConfigurationBackup` owns the file codec; `ConfigurationService` remains the authority for
validation and NVS persistence. The Web adapter uploads a bounded file, validates it, shows a
summary and requires confirmation before applying it. Omitted passwords preserve the target
credentials. Included passwords are explicitly requested and stored in plaintext in the download.

Restore validates the complete candidate, including hardware occupancy and Controller/Value
references, before writing. It writes a complete inactive NVS bank, verifies readback, then commits
a single active-bank pointer. Failed writes and interrupted staging leave the previous bank active.
Legacy `weather` configuration remains readable until the first successful restore. Subsequent
ordinary configuration changes use the active bank. Factory reset clears the active bank, inactive snapshots and legacy credentials.

The Web UI separates Device settings, Configuration backup/restore and Hardware provisioning.
The compact internal snapshot omits JSON field names to conserve NVS space. Both active and
staging banks share the existing 20 KiB partition. If there is insufficient space, restoration
fails without activating the candidate; the file size limit is not a guarantee of available NVS
capacity. Existing partition offsets remain unchanged.

Restore requests a device restart; the user can restart immediately or later. Installation network
settings from a backup become effective after restart and can change how the device is reached.

The v3 file is a flat JSON object with string-valued, named fields; display pages and enum values
reuse their existing versioned records inside JSON strings. All required fields must be present;
unknown, duplicate, malformed and unsupported fields are rejected. Fields may appear in any order.
The maximum file size is 96 KiB. The firmware version is informational; the board profile must
match and the current firmware must support all configured implementations.

License: LicenseRef-EnvNode-Use-and-Redistribution-1.0; see the root LICENSE.

## Optional location

Device settings accept latitude (-90 to 90 degrees), longitude (-180 to 180 degrees)
and sensor altitude (-1000 to 10000 metres). These fields are independently optional;
empty means unknown, while zero is a configured value. Enter the actual sensor height
above sea level, including the building or mounting height. There is no fixed outdoor
temperature setting.

Location is stored as a single NVS record so a failed write cannot mix coordinates
from different submissions. Changes request a Sensor runtime rebuild; use the existing
apply action or restart the device to activate them and clear cached measurements.
The running composition retains its previous altitude until activation succeeds.
Backups and restore previews include location. Version 2 of both JSON backups and
positional NVS snapshots added explicit presence flags and coordinate values. Version 1
is still accepted and imports all three fields as absent, even if the target device
previously had a location. Normal edits can clear location restored from a snapshot.
Factory reset clears it too. Older firmware cannot import backups newer than its supported format.

## Pressure temperature input

Device settings store a device-wide local `temperatureSensorId` (0 = no temperature)
and `maxTemperatureAgeMs` (positive, at most INT32_MAX; default 60000 ms).
The Web UI edits the age in seconds, with millisecond precision. The selected slot
must be enabled, have an enabled schedule and expose the `temperature` channel.
Disabling or changing it incompatibly is rejected until the selection is cleared.
The two settings are persisted atomically in one NVS key (`pressureReduce`), survive
reboot and OTA, and participate in reset, restore validation and restore preview.
Sensor runtime activation takes an active snapshot and clears measurement caches.

Backup/snapshot version 3 adds the temperature source and maximum age. Versions 1
and 2 remain readable; they clear temperature selection and use the default age.
Version-2 location fields are retained; version 1 still means unknown location.

### Locale and numeric inputs

Firmware 0.6.1 location and pressure-temperature age inputs follow the configured
Locale (German commas, English points), without thousands separators. The Web
boundary converts these strings to neutral numeric input; application configuration,
NVS, backups and MQTT remain independent of the UI Locale. No format migration is
needed. Restore previews and temperature-age diagnostics are localized too.

## Restore preview memory and diagnostics

The upload/preview stage does not write NVS. It decodes and validates a candidate;
only the later explicitly confirmed restore stages/commits a bank. A browser
connection reset during upload/preview does not by itself identify a storage fault.

The JSON decoder validates/counts fields before allocating an exact-size index
with checked nothrow allocation. It no longer grows an unchecked vector while the
uploaded JSON occupies heap. The web preview takes ownership of the single decoded
Configuration rather than keeping an additional full destination model, and frees
the JSON before domain validation/rendering. The existing decoder API remains
available; format versions, input limits, validation and credential handling remain.

INFO diagnostics mark upload start/completion, preview decode, domain validation
and response completion, including bytes, elapsed time, free heap and largest
free block. They contain no uploaded credentials or configuration contents.
A simulated index-allocation failure is covered natively; physical constrained
heap/network behavior still requires the device retry and these phase logs.
