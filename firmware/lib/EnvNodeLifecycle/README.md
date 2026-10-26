# EnvNodeLifecycle

Shared OTA upload state machine and device-restart execution used by the existing EnvNode application. No second firmware application is introduced.

OTAService depends only on ILogger, IDeviceRestart and the Arduino Update backend. Successful validation/staging requests a device restart through the interface; it never executes the restart. Known/unknown image-size handling, byte counters, state names, error messages, abort and retry behavior are unchanged. Existing OTAService include names and construction with RuntimeManager remain source-compatible.

IDeviceRestart separates requesting, inspecting and explicitly executing restart. EnvNode RuntimeManager implements this contract, retaining its existing pending-action priority and sensor/actuator/controller apply logic. It delegates only the authorized hardware operation to DeviceRestartExecutor.

DeviceRestartExecutor owns the single ESP.restart() call and the existing execution log message. It does not decide when a restart is allowed. Other services must request restart via IDeviceRestart, not construct an executor to bypass application policy. A driver can be injected for native tests; the default driver performs ESP.restart() on ESP32 and remains a no-op on native.

EnvNode WebService still owns firmware/admin HTTP handlers and factory-reset storage policy. Restart and restore handlers now use the narrow restart interface, preserving response-before-flush-before-restart ordering and blocking restart/reset while OTA is busy. Network, NVS and partition layout are unchanged.

EnvNode-owned files follow the repository LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
