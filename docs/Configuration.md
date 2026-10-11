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

The v1 file is a flat JSON object with string-valued, named fields; display pages and enum values
reuse their existing versioned records inside JSON strings. All required fields must be present;
unknown, duplicate, malformed and unsupported fields are rejected. Fields may appear in any order.
The maximum file size is 96 KiB. The firmware version is informational; the board profile must
match and the current firmware must support all configured implementations.

License: LicenseRef-EnvNode-Use-and-Redistribution-1.0; see the root LICENSE.
