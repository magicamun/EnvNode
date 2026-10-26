# EnvNodeBaseConfiguration

Shared device, network, time and locale data types plus the existing validation rules. This library has no sensor, actuator, controller or board-profile dependencies. It uses Arduino String and IPAddress; native tests use the existing support doubles.

INetworkConfiguration is a read-only view onto application-owned network settings. EnvNode IConfigurationService implements it by returning getConfiguration().network, so existing applications and configuration test doubles need no new methods.

Defaults, NVS keys/namespaces, atomic restore banks, backup versions and the complete Configuration aggregate remain in the EnvNode application in this step. This is a shared data/validation boundary, not yet a second persistent configuration service. A future tester service can implement the small interface without bringing in the EnvNode configuration aggregate.

Locale header path, enumeration values and key parsing are unchanged. Existing consumers can continue to include Configuration.h or EnvNode/Locale.h.

EnvNode-owned files follow the repository LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).

ILocaleConfiguration and ITimeConfiguration are narrow read-only views;
IBaseSettings combines only device/time/locale access and their existing setters.
LocaleFormatter uses the locale view directly, retaining all date, time, grouping
and precision behavior. IConfigurationService adapts the EnvNode model to these
interfaces. This does not yet extract Preferences persistence or change any
stored settings/schema/defaults.
