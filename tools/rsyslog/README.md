# EnvNode UDP receiver example

EnvNode-owned: see repository LICENSE.

Prepared for `iot-rsyslogd` / `192.168.20.10`, UDP 514. These files have **not**
been installed or executed on that host. `rsyslogd`, Docker and Podman are not
available in the development environment, so syntax/rotation verification on
rsyslog is still pending. Adapt the example to the target distribution first.

1. Confirm UDP port 514 and the server's bind address. Prepare `/var/log/envnode`
   with the runtime rsyslog user's ownership and directory permissions 0750.
2. Use envnode.conf as a standalone config, or integrate its ruleset/listener into
   the server configuration. Avoid a duplicate imudp module load/global option.
   This listener routes only its own input; other server logging stays independent.
3. Validate the **complete intended config** with `rsyslogd -N1 -f <config>` before
   installing/reloading it. No deployment is performed by the firmware build.
4. Permit UDP 514 from the intended device network. UDP is unauthenticated;
   the factory ID identifies records but does not authenticate the sender.
5. Configure the firmware at `/network-logging`: numeric IP `192.168.20.10`,
   port `514`, enabled, system logs and desired diagnostic sources. Select
   RainDetector diagnostics in its sensor settings and its controller separately.
6. Expect valid JSONL in system.jsonl and diagnostics.jsonl. A packet contains a
   complete JSON object, no batching. Check device_id, boot_id, build, sequence,
   uptime_ms and optional epoch_s; a successful local send is no delivery receipt.
7. Adapt logrotate owner/group and service name. Run `logrotate --debug <config>`;
   verify rename/reopen with test data before enabling scheduled rotation. Daily
   plus maxsize requires running logrotate frequently enough for the desired
   size bound; 32 MiB is a trigger, not a hard filesystem limit. HUP follows rename
   so rsyslog reopens the files. No copytruncate is used.

Normal rain datagrams fit below 1,200 bytes. The complete generic event contract
allows up to 2,047 bytes of wire content, so extreme escaped-text events can be
IP-fragmented on a 1,500-byte MTU. This preserves existing field semantics; an
entire fragmented datagram can be lost. The receiver maximum is 4 KiB. A smaller
MTU is not guaranteed by this transport.

Check gaps independently for each `(device_id, boot_id, type)` stream. Diagnostic
sequence numbers count selected valid submission attempts, including full/busy
queue rejection. Disabled sources do not consume sequence numbers. Sequence and
uptime counters use unsigned 32-bit wrap; compare modularly, and use rain sensor
revision/start_us/end_us for acquisition continuity. Concurrent submissions can
be received out of sequence; UDP itself may reorder or duplicate packets. There
is no persistent replay after a reboot/outage. Sender counters cannot identify
silent server/network drops, including a lost tail with no subsequent record.

References: [imudp ruleset/listener](https://docs.rsyslog.com/doc/configuration/modules/imudp.html),
[omfile and reopening after rotation](https://docs.rsyslog.com/doc/configuration/modules/omfile.html).

For a captured interval, run the repository helper on copies of the receiver files:

```sh
python3 tools/analyze_udp_logging.py system.jsonl diagnostics.jsonl
```

It reports per-device/boot/stream sequence gaps and duplicates, rain revision
gaps/resets, gate-duration range and observed window rate. Sequence ordering
allows UDP reordering and uint32 wrap within half the sequence range. This is
an interval summary: it cannot discover a missing prefix/tail or unobserved boot.
Retain raw files alongside the summary. No server file is modified by the tool.
