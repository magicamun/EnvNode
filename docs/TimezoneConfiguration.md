<!-- EnvNode-owned documentation; LicenseRef-EnvNode-Use-and-Redistribution-1.0; see ../LICENSE. -->
# Timezone configuration

The Locale & Time page offers all 597 identifiers (including compatibility aliases)
from IANA tzdata 2026e, sorted alphabetically. It works offline. The web page
streams the selector in roughly 1 KiB chunks to avoid several simultaneous large
HTML allocations on the ESP32. Names such as
`Asia/Kathmandu`, `Australia/Lord_Howe`, and `Pacific/Chatham` include fractional
UTC offsets and, where applicable, recurring daylight-saving rules.

The selected identifier maps to a POSIX TZ rule. ConfigurationService stores this
rule in NVS; TimeService applies it through `TZ` and `tzset()`. Locale remains
independent. Existing installations and configuration backups require no migration:
the seven original rule strings are preserved. Several identifiers share a rule;
after saving, the selector displays the first alphabetical identifier with the
stored rule. The firmware stores the rule, not a geographic identifier.
Unrecognized stored rules remain visible as a custom option.

## Limits

This is a complete identifier selection, not a full IANA transition engine. POSIX
rules express fixed offsets or annual recurring transitions. They cannot reproduce
historical changes or irregular future transitions such as Morocco's Ramadan
exceptions. The generated rules are the TZif footer's future recurring approximation.
For such regions, local civil time may differ from the official time during an
exception. A full transition engine would require a separate architecture change.
Political changes to timezone rules require updating the catalog and firmware;
existing saved POSIX rules must be reselected to adopt changed rules.

## Updating the catalog

Download the desired official release from
<https://data.iana.org/time-zones/releases/> and extract it into a temporary directory.
Compile all main data files and aliases with `zic`, then run the generator:

```sh
zic -d /tmp/envnode-zoneinfo \
  /tmp/envnode-tzdata/africa /tmp/envnode-tzdata/antarctica \
  /tmp/envnode-tzdata/asia /tmp/envnode-tzdata/australasia \
  /tmp/envnode-tzdata/europe /tmp/envnode-tzdata/northamerica \
  /tmp/envnode-tzdata/southamerica /tmp/envnode-tzdata/etcetera \
  /tmp/envnode-tzdata/backward
python3 firmware/scripts/generate_timezone_options.py /tmp/envnode-zoneinfo 2026e
```

Use a fresh output directory to avoid retaining removed identifiers. Commit the
generated `firmware/include/TimezoneOptions.inc`, update the version and count in
this document, and build the firmware. No download occurs during a firmware build
or at device runtime.

The generated timezone data is public domain under the IANA tzdata LICENSE:
“Unless specified below, all files in the tz code and data (including this LICENSE
file) are in the public domain.” The BSD exceptions concern `date.c`,
`newstrftime.3`, and `strftime.c`; none is included here. EnvNode's generator and
documentation follow the root LICENSE; timezone data retains its original status.
