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

## Hardware checkpoint: build 478

The user authorized flashing the candidate on Weather 0.2, serial
/dev/cu.usbserial-0001. ESP32-D0WD-V3 revision 3.1 and MAC 30:76:f5:e5:c6:14 were
confirmed. The prior application was 0.8.7+477.gc08a626. A fresh table/NVS/OTA read
showed the current large layout, but OTA sequence 1 selecting app0 at 0x10000,
unlike the earlier integration tests' app1. OTA states were UNDEFINED (rollback
not enabled), with valid sequence CRCs. The whole active 0x1c0000-byte app0 was
saved before writing. Only the new app was written at 0x10000; esptool verified
its data hash. A subsequent read confirmed byte-identical partition-table and
OTA regions. No bootloader, configuration, server or firewall change was made.

The device reports **0.8.7+478.gecc43ac**, clean source. Linker sizes are
1,569,565 bytes flash and 100,792 bytes static RAM; the final binary is 1,576,144
bytes. Logger, sensor slot 1 and controller slot 1 form fingerprints match
before/after. The full-rate rain source is slot 4 in receiver event identities.

Ten complete requests per route before flashing and two ten-request rounds after
flashing yielded these median complete-response times (milliseconds):

| Route | Build 477 | Build 478 first round | Build 478 warm round |
| --- | ---: | ---: | ---: |
| / | 84.8 | 101.8 | 103.5 |
| /sensors | 117.9 | 141.1 | 151.1 |
| /controllers | 123.1 | 149.7 | 146.4 |
| /measurements | 124.1 | 146.5 | 150.8 |
| /display | 84.5 | 105.4 | 105.4 |
| /style.css | 74.2 | 91.5 | 97.5 |

All 180 sequential requests returned complete HTTP 200 responses. The largest
observed total was 419.7 ms after flashing, versus 296.6 ms before. A concurrent
controller-page/CSS pair also completed; order affects the single-client server's
queueing. A separate client requested /controllers with a 1-KiB receive buffer
and deliberately did not read for five seconds. A concurrent status request
completed in 190.4 ms before and 209.3 ms after. That client did not cause a long
stall in this setup; it does not prove arbitrary backpressure is harmless.

**This hardware comparison does not demonstrate a speedup.** Candidate medians
were slower in both rounds; network/boot-phase differences are not isolated by a
randomized repeated A/B experiment. The user-visible intermittent main-page hitch
has not been reproduced by these requests. Keep this as an experimental branch;
do not promote it to main as a confirmed performance fix. Browser observation
and better per-service/per-response profiling remain the next evidence needed.

After load, internal 8-bit heap was 38,264 bytes free, 14,656 minimum and 19,444
largest block. Observed logger service maximum was 11,133 us; application body
maximum was 454,382 us. Application latency remains, despite bounded HTTP sends.
The final receiver sample spans 165.6 seconds: 614 diagnostic events, including
599 rain windows, with zero diagnostic sequence gaps, zero duplicates and zero
rain revision gaps/resets. All 599 windows have fault=none. Observed rate is
3.637 windows/s; gates span 250,041–524,406 us, with maximum adjacent end delta
403,921 us. Nominal 250-ms acquisition remains subject to existing main-loop
latency; this is not an exact 4-Hz deadline result. Controller configuration,
decision/output and runtime-health events were received. The existing 18 startup system-log
queue drops remain explicitly visible; they are not diagnostic-window losses.

Private backups, request timing JSON, flash/read logs and receiver JSONL remain
Git-ignored in firmware/.pio/logging-validation/web-speed/. Candidate firmware,
ELF, build identity and checksums are in firmware/bin/web-speed/build478/.

## Browser reproduction: idle TCP connection starves the next request

Safari's Web Inspector reproduced the visible main-page hitch on the installed
build 478. Clicking Controllers from Measurements showed 5,222.8 ms total:
64.0 ms TCP establishment, 5,056.6 ms waiting for the response, and 100.5 ms
loading it. CSS came from memory in 0.06 ms. This captured delay precedes the
response body; it is not explained by slow CSS transfer or page painting.

The installed Arduino WebServer has HTTP_MAX_DATA_WAIT=5000. handleClient accepts
one client, retains it in HC_WAIT_READ while it has no available data, and does
not accept another until that wait expires. A controlled read-only socket test
opened one TCP connection, sent no bytes, waited 300 ms, then issued an ordinary
/controllers GET on a second connection. Headers arrived after 4,746 ms and the
full body after 4,839 ms. After closing the idle connection, the same request
completed in 145 ms. Both responses were HTTP 200 with 13,082 body bytes.

This demonstrates request starvation by an accepted empty TCP connection.
Safari speculative connections are a plausible trigger, but were not identified
by packet capture. The captured Safari symptom and controlled starvation have
the same five-second signature. The next fix should address selection/fairness
among waiting connections, preserving bounded RAM and normal slow/fragmented
requests. Increasing the response-body budget does not address this wait.
No firmware/configuration/server change accompanied this browser diagnosis.
Screenshot evidence remains private/Git-ignored at
firmware/.pio/logging-validation/web-speed/safari-inspector.png.

## Targeted fix: bounded waiting clients before the SDK parser

MeasuredWebServer now selects a client only after its first request byte is
available. A fixed HttpClientGate holds at most two waiting client handles, uses
one nonblocking WiFiServer::available accept per application turn, and never
reads/buffers request bytes. Waiting clients with data are selected round-robin
before new arrivals. A new ready client can be served even while both waiting
slots contain empty connections. If another empty connection arrives at a full
pool, the oldest still-empty one is closed; ready clients are not evicted. A rare
arrival with no room and no empty eviction candidate is rejected. Expiry,
eviction and rejection counters are retained in the gate. A five-second
first-byte deadline and disconnected-client cleanup bound handle retention;
unsigned time subtraction handles millis wraparound. Idle polling retains the
SDK's one-millisecond cooperative delay.

This is a project-owned adapter using the existing WebServer protected client
and state interface, not a patch/copy of the framework's parser. Only ESP32
builds use this accept adapter. Route handling, upload parsing and deferred
responses keep their existing implementation. Request bytes remain untouched,
including the first byte of fragmented requests. Once bytes arrive, the existing
synchronous SDK parser can still block on incomplete headers/uploads. This change
addresses the demonstrated empty-connection starvation, not every slow-client
or synchronous MQTT/application stall. The prior 8-KiB response-pump experiment
remains separately identifiable in ecc43ac; it is not the fix for the five-second
pre-response wait.

Native validation of the targeted fix passed 52 cases: 23 WebHost/client-gate,
three firmware-runtime, 20 RainDetector policy/acquisition/controller, six UDP
logging cases. Added cases exercise idle-plus-ready clients, later/fragmented
first bytes, a full idle pool, ready-client fairness, timeout wraparound and
closed-client cleanup. After a hub disconnect and NAS executable-busy errors,
the successful run used PLATFORMIO_BUILD_DIR=/private/tmp/envnode-web-gate-build;
failed/interrupted earlier runs are not counted as validation.

The read-only hardware regression is reproducible with:

```sh
python3 tools/probe_http_idle_clients.py http://192.168.20.63/controllers
```

On build 478 the controlled baseline returned 415.6 ms with no idle clients,
4,840.5 ms with one, 9,887.0 ms with two, timed out with four (10-second socket
timeout), and took 4,915.3 ms for one idle client plus a fragmented request.
The probe checks status 200 and a configurable total-time limit (default one
second). This is an integration test against a supplied read-only URL; it does
not change device configuration or any server settings.
