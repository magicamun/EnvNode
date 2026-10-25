# EnvNodeBaseConfiguration

Shared device, network, time and locale data types plus the existing validation rules. This library has no sensor, actuator, controller or board-profile dependencies. It uses Arduino String and IPAddress; native tests use the existing support doubles.

INetworkConfiguration is a read-only view onto application-owned network settings. EnvNode IConfigurationService implements it by returning getConfiguration().network, so existing applications and configuration test doubles need no new methods.

Defaults, NVS keys/namespaces, atomic restore banks, backup versions and the complete Configuration aggregate remain in the EnvNode application in this step. This is a shared data/validation boundary, not yet a second persistent configuration service. A future tester service can implement the small interface without bringing in the EnvNode configuration aggregate.

Locale header path, enumeration values and key parsing are unchanged. Existing consumers can continue to include Configuration.h or EnvNode/Locale.h.

EnvNode-owned files follow the repository LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
