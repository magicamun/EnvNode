# EnvNodeLogging

Shared local PlatformIO library for the EnvNode and future test-board applications. Public header names and C++ interfaces are unchanged. PlatformIO discovers this library through existing logging includes; no registry dependency is required.

Contains ILogger, log levels and entries, sink/time/reader interfaces, the bounded recent-log store, StructuredLogger, timestamp formatting and the Arduino SerialLogger adapter. Only SerialLogger requires Arduino; native builds use the existing test/support/Arduino.h stub.

SystemLogTimeProvider remains in the application because it adapts IMonotonicClock and system time. LogWebView remains in the web layer because it depends on Arduino HTML rendering and WebLocalization. Neither is a dependency of this library.

This extraction changes file ownership and build integration only: no API, message format, buffer size, persistence or runtime behavior changes. Future changes must keep sensor-branch consumers compatible or document an explicit migration.

EnvNode-owned files follow the repository LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
