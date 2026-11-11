# EnvNode UDP receiver and source separation

EnvNode-owned: see repository LICENSE.

Installed on `iot-rsyslogd` (`192.168.20.10:514/UDP`) on 2026-11-06 at
20:24:52 UTC. Receiver: Debian, rsyslog 8.2302.0. No firmware change required.

## Files and routing

The existing `/etc/rsyslog.d/20-remote-udp.conf` still owns the **only** UDP
listener and writes every packet to `/var/log/remote/<source-IP>/syslog.log`.
Its `RemoteUDP` ruleset additionally calls `EnvNodeJsonSplit` before `stop`.
The add-on is installed as `/etc/rsyslog.d/10-envnode-json.conf`.

New files are created on receipt under `/var/log/envnode/<device_id>/`:

| File | Content |
| --- | --- |
| `system.jsonl` | System stream |
| `diagnostics.jsonl` | Complete valid diagnostic stream |
| `by-source/sensor-4.jsonl` | Example: sensor slot 4 |
| `by-source/controller-1.jsonl` | Example: controller slot 1 |
| `by-source/<source_kind>-<source_id>.jsonl` | Each system/sensor/actuator/controller/module source |
| `rain-detector.jsonl` | Events whose name starts with `rain_`, including controller decisions |
| `controllers.jsonl` | All controller-source diagnostics |
| `test-markers.jsonl` | Events named `test_marker` |

These are overlapping views: markers also remain in diagnostics and their
source file; controller rain events also appear in the rain view. Original JSON
message content is written without reserialization, averaging or rate reduction.
All configured source slots are handled dynamically; sensor 4 is not hard-coded.
Device ID is exactly 12 lowercase hexadecimal characters. Source paths allow
system/0 and sensor, actuator, controller, module/1..32 only. Failed JSON, invalid
device identity, stream/type mismatch or invalid source metadata go to
`/var/log/envnode/unrouted.log`. Valid diagnostic records with unsupported source
metadata also remain in `diagnostics.jsonl`. Other applications remain in the
original remote log without entering EnvNode views. UDP device identity is not
authentication; retain appropriate network access restrictions.

**Views begin at activation; historical logs are not automatically backfilled.**
Existing raw logs and their rotation were left unchanged. No new marker file
appears until a marker arrives. At deployment the most recent real device packet
was from 16:43:11 CET, hours before activation; fresh-device reception and restart
sequence continuity could therefore not be verified. The receiver is active and
bound to UDP 514. Verify the new files when the device resumes sending.

```sh
tail -f /var/log/envnode/14c6e5f57630/rain-detector.jsonl
tail -f /var/log/envnode/14c6e5f57630/test-markers.jsonl
```

## Deployment and rollback

`10-envnode-json.conf` is an **add-on without a listener**. Integrate its call into
the existing UDP ruleset, after the raw-file action. Do not install the standalone
`envnode.conf` alongside an existing listener. That file is only an alternative
minimal standalone example. `cookie=""` supports plain JSON on the installed
version; newer `find-json` parsing mode is not required.

Before deployment run the isolated test below, validate the complete candidate
configuration with `rsyslogd -N1 -f <candidate>`, and check rotation with
`logrotate --debug /etc/logrotate.conf`. Install configuration root:root 0644;
output directories root:adm 0750, files 0640. Validate again before restarting.
A service restart loads configuration; HUP only reopens output files. UDP packets
can be lost during restart. No persistent delivery guarantee is provided.

The deployment backup is:
`/var/backups/rsyslog-envnode/20261106T202452Z/`.
It contains original `20-remote-udp.conf`, `rsyslog.conf`, `rsyslog-remote`, an
absence manifest for the two added configuration files and a raw-log checkpoint.
To roll back this deployment: restore its `20-remote-udp.conf` to
`/etc/rsyslog.d/20-remote-udp.conf`, remove only the newly added
`/etc/rsyslog.d/10-envnode-json.conf` and `/etc/logrotate.d/envnode`, validate with
`rsyslogd -N1`, then restart rsyslog. Retain all data files. Review subsequent
configuration edits before restoring an older backup.

## Rotation

Install `envnode.logrotate` as `/etc/logrotate.d/envnode`. The existing Debian
daily `logrotate.timer` handles it: 14 archives per nonempty file, compression
with one rotation delay, rename and HUP/reopen, no copytruncate. Daily file sizes
depend on incoming volume; this is not a hard size or total storage limit.
Multiple views intentionally increase server storage use. Raw-log rotation is
independent and unchanged. Full production logrotate debug validation passed.

## Validation

Run on a Debian receiver as root with rsyslogd, logrotate and Python 3:

```sh
python3 tools/rsyslog/test_split.py
```

The test uses a temporary directory, its own daemon/PID and a loopback-only UDP
port. It does not restart services or modify production logs. It validates system,
sensor, controller, actuator, module, marker and rain routing; multiple devices;
malformed JSON and unsafe path fields; preservation of UTF-8, quoting and large
integer JSON bytes; and forced rotation plus HUP/reopen using isolated state.
All checks passed on the receiver on 2026-11-06. Complete production syntax and
logrotate debug checks also passed. Real-device confirmation remains pending
because no current device traffic was present at deployment.

## Sequence checks

Use the complete `diagnostics.jsonl` for diagnostic sequence-gap checks, **not**
a filtered source/event view: omitted events produce intentional gaps there.
On local copies of complete stream files:

```sh
python3 tools/analyze_udp_logging.py system.jsonl diagnostics.jsonl
```

Check independently per `(device_id, boot_id, type)`. Submission sequences include
full/busy queue rejection, while disabled sources consume no sequence numbers.
Counters wrap at uint32; UDP can reorder, duplicate or drop packets. A captured
interval cannot reveal a missing prefix/tail or an unobserved boot. Rain revision
and start/end times additionally describe acquisition continuity. No replay is
available after outages. Retain raw files alongside analysis results.

References: [plain JSON parsing](https://docs.rsyslog.com/doc/reference/parameters/mmjsonparse-cookie.html),
[UDP listener](https://docs.rsyslog.com/doc/configuration/modules/imudp.html),
[omfile and rotation](https://docs.rsyslog.com/doc/configuration/modules/omfile.html).
