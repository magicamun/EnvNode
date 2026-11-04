<!-- SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0 -->
<!-- See the repository root LICENSE. -->
# Web response performance

## Initial transport change, 2026-11-04

The validated UDP/no-SD baseline already defers normal HTML and CSS delivery.
Its response pump sends at most 2,048 bytes in four calls per application loop.
The hardware validation recorded application-body maxima around 450 ms. A slow
application loop therefore also limits page throughput, even when the TCP socket
has capacity. This is a transport scheduling constraint, not proof that HTTP is
the cause of the slow application body.

The next candidate increases the per-turn budget to 8,192 bytes/eight nonblocking
socket calls, with individual writes limited to 1,460 bytes. A two-millisecond
elapsed-time guard stops work between socket calls. A call already in progress
cannot be preempted by this guard; the ESP32 sink continues to use MSG_DONTWAIT.
Temporary backpressure returns immediately. No response buffer, task or second
page copy is added. Existing response-size, partial-write and timeout behavior
remains. Measurements, controllers and full-rate UDP diagnostic production keep
their existing scheduling and semantics.

Slow-response completion and abort logs additionally contain:

- `steps`: number of response-pump invocations;
- `blocked_steps`: invocations returning More without accepting any bytes;
- `max_gap_ms`: longest interval between pump invocations (first interval starts
  when the response is queued);
- `max_step_ms`: longest measured pump invocation, including response cleanup.

These fields distinguish application scheduling gaps from socket backpressure.
They are not CPU-utilization measurements. The route is limited to 40 characters
in this diagnostic so all counters fit the existing 256-byte system-log limit. The existing log threshold (one second
for completed responses) remains, avoiding a new log entry for every quick poll.

## Validation and next hardware comparison

Native regressions cover exact bytes/length, bounded bytes and call count,
partial writes, blocked/disconnected/failed clients, timeout and millisecond
wraparound, setup-AP mode and deferred restart. A deterministic uncongested
32-KiB-plus-header fixture with one pump invocation every 250 ms needs five turns
(1.25 s) rather than the previous budget's 17 turns (4.25 s). This is a scheduling
fixture, not a claimed hardware speedup or WLAN throughput measurement.

Before accepting the candidate on hardware, compare identical routes and page
sizes against the tested baseline: time to first byte, full response duration,
repeated requests, a nonreading client and simultaneous full-rate diagnostics.
Check `max_gap_ms`, `max_step_ms`, minimum/largest heap, application/logger maxima,
transport sequences and rain-window revisions/gate lengths. Higher socket-buffer
occupancy may consume more runtime heap despite unchanged application buffers.
No hardware flash, receiver change or reduction in diagnostic rate is part of
this initial local change. If gaps dominate, profile individual application
services next; synchronous MQTT connect/publish and request parsing also remain
separate possible blocking paths.

## Local checks and unchanged-device baseline

Native checks: 18 WebHost tests, three firmware-runtime ordering tests and six
UDP logging tests pass, including real loopback delivery of 14,400 windows. All
20 RainDetector policy/acquisition/controller regressions also pass.
The ESP32 candidate builds on the existing Arduino 2.x toolchain and large OTA
partition layout. Final linker figures: 1,569,581 bytes flash and 100,792 bytes static RAM. The
validated no-SD reference reported 1,569,321 and 100,776 bytes (+260/+16 bytes).
Build identity differences are included; this is not an isolated attribution of
every flash byte to the transport change. Candidate runtime heap is unmeasured.

Read-only GET requests to the unchanged running device at 192.168.20.63 produced:

| Route | Body bytes | Headers received, ms | Complete, ms |
| --- | ---: | ---: | ---: |
| / (three requests) | 5,122 | 49–180 | 87–211 |
| /style.css (three requests) | 7,810 | 36–37 | 71–78 |
| /sensors | 10,740 | 66 | 122 |
| /controllers | 13,082 | 55 | 139 |
| /display | 6,657 | 166 | 213 |
| /diagnostics | 7,180 | 54 | 96 |
| /logs/data | 3,547 | 34 | 41 |

These eleven sequential samples did not reproduce a long stall. Header timing
includes connection setup and is not an isolated rendering measurement. No page
contents, configuration or credentials were saved. The candidate has not been
flashed, so none of these times measure the new transport. Browser rendering,
parallel asset requests, uploads and long-running tail latency remain unmeasured.
