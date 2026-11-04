# 0016: Classic ESP32 network diagnostics and hardware profiles

EnvNode-owned: see repository LICENSE.

Decision: accepted, 2026-11-04.

The classic ESP32 retains the validated Arduino 2.x / espressif32 7.0.1 build.
SD long-term logs/diagnostics move to a configurable rsyslog receiver over UDP.
SD runtime, file management, web routes, formatting, SD tests and the associated
framework patch are removed after validating the UDP logger with every existing
RainDetector measurement window. SX1262 is not a supported classic driver.

Producer event names, units and typed fields remain stable. The shared
IDiagnosticRecorder boundary, diagnostic checkboxes and saved preferences remain.
Network admission is independent of WiFi/receiver reachability: producer calls
only copy into bounded queues, while a cooperative nonblocking socket step has
bounded work. Offline/full/busy/send/encoding losses are counted and visible.
There is no durable replay, averaging or downsampling to control traffic. Local
socket acceptance never promises receipt. Each packet carries device/boot/build
identity, stream sequence and capture time; sensor revisions identify window gaps.

The existing schema-7 backup layout retains its four legacy SD fields solely for
old positional snapshot compatibility. New exports set SD enabled=false/slot=0;
legacy diagnostic choices migrate to network settings. These compatibility fields
cannot reserve hardware or activate a driver. Installed unsupported module
descriptors remain visible and their physical resources remain protected from
accidental reuse, independently of firmware driver support.

The application core stays shared. The current weather application is composed
by esp32dev, and the testboard keeps its separate composition with shared
libraries. A later S3 hardware/build profile should bring its new framework,
SD and SX1262 driver implementations together, through these same application
interfaces. No nonfunctional S3 build target or copied application is introduced
in advance. Hardware capabilities and provisioning data are independent of
which build contains a runtime driver.

RainDetector remains included. Analog acquisition already has shared/native
coverage; integrating additional analog hardware and SCT013 requires per-scenario
measurements of flash, internal 8-bit heap, largest block, task stack and service
latency, including the combined load. Passing a static link or an isolated sensor
test is insufficient to promise that all features fit together. The measured
HTTP/TCP-related memory pressure remains part of that budget.

See UdpLoggingImplementationPlan.md and UdpLoggingValidation.md for phased
commits, resource measurements, receiver evidence and physical limitations.
Historical SD code is reachable at commit 6bb9b1c before removal; Git history is
preserved. Receiver examples under tools/rsyslog separate system/diagnostic files
and define rotation, while live server configuration remains an explicit
operational deployment step.
