# EnvNodeTime

EnvNode-owned materials follow the repository LICENSE
(LicenseRef-EnvNode-Use-and-Redistribution-1.0).

Shared TimeService/ITimeService with read-only ITimeConfiguration, IWiFiService
and ILogger dependencies. No EnvNode configuration model, sensors, board profile,
web server, Preferences store or application runtime is required. Existing public
include names and construction through IConfigurationService remain valid.

The existing behavior is retained: wait for WiFi, stop an existing SNTP instance,
configure polling and two NTP servers, clear server slot 2, start SNTP and install
the configured POSIX timezone. Empty fields use the original defaults. A later
loop marks the time synchronized after the existing 2021 timestamp threshold.
Explicit begin() resets the service state; this extraction adds no reconnect or
retry policy. UTC/local ISO output and timezone offsets are unchanged.

LocaleFormatter lives in EnvNodeBaseConfiguration and only reads
ILocaleConfiguration. Settings storage and decisions to restart services remain
application-owned. Native tests use a fake SNTP API and host time/timezone
functions; they do not contact an NTP server or prove physical synchronization.
