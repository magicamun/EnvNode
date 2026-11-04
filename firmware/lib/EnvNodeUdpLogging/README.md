# EnvNodeUdpLogging

EnvNode-owned: see repository LICENSE.

Portable RFC5424/JSON encoding and bounded queues. Hardware producers still use
IDiagnosticRecorder. The ESP32 application owns numeric address configuration,
boot identity and the nonblocking socket adapter. Serial and the recent RAM log
store remain independent.

There are eight system entries, the existing eight diagnostic records and one
2-KiB encoding buffer, allocated once after runtime startup. service() removes
at most one entry from each stream. No persistent retry, blocking producer lock,
per-event application allocation, DNS, batching or downsampling is used. Failed
transport attempts are counted and dropped. The application calls service once
per main loop; this needs hardware timing validation under simultaneous HTTP,
MQTT, display and controller load.

Diagnostics retain the existing JSON schema and fields with additive identity
metadata. Each message has a factory device ID, random 128-bit boot ID, firmware
build, stream type, sequence and original capture time. Diagnostic sequence
allocation precedes potential queue rejection. Invalid or disabled events do not
consume a sequence; selected submissions rejected as unavailable/full/busy do.
Counters and sequence numbers wrap unsigned at 32 bits. Concurrent producers and
UDP can reorder messages. A lost tail cannot be detected until another event or
local counter observation; local send success is never proof of server receipt.

The 2,048-byte encoder capacity preserves maximum escaped fields (including
complete 256-byte system-log storage and six typed diagnostic fields). The
largest valid message is at most 2,047 bytes. Ordinary rain events are under
1,200 bytes; extreme generic events can require IP fragmentation on normal MTU.
Oversize/invalid encodings return zero and increment encode_error, never emit a
partial JSON document. A future fragmentation-free protocol must explicitly
handle this existing contract rather than truncate it.

Tests: `pio test -e native -f test_udp_logging`. This includes fake transport
errors/full queues, worst-case encoding, accelerated one-hour 250-ms production
with controller bursts, and a real local UDP loopback for all 14,400 windows.
The loopback test only contacts 127.0.0.1; it does not validate WiFi or rsyslog.
