# EnvNodeNetwork

Shared ESP32 WiFi service and IWiFiService interface. WiFiService requires only ILogger and INetworkConfiguration. It does not include IConfigurationService, Configuration or sensor/actuator/controller types.

Existing construction with ConfigurationService remains source-compatible through the inherited read-only adapter. The service still owns station connection, reconnect attempts, setup-AP fallback, static address application and timeout scan diagnostics. Credentials, timeouts and setup-AP naming retain their previous behavior. Existing include names WiFiService.h and IWiFiService.h remain unchanged.

PlatformIO discovers the local EnvNodeLogging and EnvNodeBaseConfiguration libraries through their public includes. Production builds use Arduino ESP32 WiFi; native lifecycle tests use test/support/WiFi.h. No production transport or timing algorithm was replaced.

EnvNode-owned files follow the repository LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
