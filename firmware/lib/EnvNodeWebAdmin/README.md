# EnvNodeWebAdmin

Network and log administration route modules used by the existing EnvNode firmware. No second application or tester behavior is introduced.

NetworkWebRoutes registers GET /network and POST /network/save. It requires only INetworkSettings, IWiFiService, IWebPagePresenter, INetworkSaveObserver and WebHost. The settings service owns validation/persistence. The observer reports the outcome to the application, which retains its existing RuntimeManager action and result-page wording. Existing password-checkbox semantics and form fields are preserved.

LogWebRoutes registers GET /logs and GET /logs/data. It uses the same page presenter, WebHost and IRecentLogReader. LogWebView retains its HTML/JSON output and refresh script. Offline JSON requests retain their 503 response; setup-AP requests remain available.

Page providers and referenced services must outlive the HTTP server. Locale is read from the presenter for each request. The modules have no SensorManager, ActuatorRuntime, ControllerRuntime, RuntimeManager or complete Configuration dependency.

The unchanged translation catalog/rendering lives in EnvNodeWeb, including existing EnvNode text IDs. JsonWriter lives in EnvNodeBaseConfiguration so configuration backup and MQTT consumers do not acquire an HTTP dependency. Existing include names remain available.

EnvNode-owned files follow the repository LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).

FirmwareWebRoutes owns `/firmware`, `/firmware/upload`, `/restart` and
`/factory-reset`. The application supplies build metadata, a configuration-reset
callback and localized number formatting. It does not expose its full
configuration or runtime manager to this library. Explicit restart flushes the
HTTP response before execution; successful OTA upload only stages the image.

HardwareProvisioningWebRoutes owns GET `/hardware` plus the existing hardware/
device aliases for board-identity and module-descriptor writes. It uses
IProvisioningAdministration for profile metadata and writes; a separate callback
renders the application-owned read-only hardware inventory. Random bytes are
injected, so this module has no ESP-specific random API dependency. The existing
DuoRelay form/template, manufacturing fields, UUID-v4 conversion, confirmation
checks, statuses and restart choices are preserved.

EnvNode keeps active-board/profile policy, module discovery and compatibility
checks. Its small provisioning route object is created on the heap at web startup
to avoid overflowing the ESP32 static DRAM segment; allocation failure is logged
and leaves the remaining web routes running. `sendWebResult` in EnvNodeWeb keeps
the common escaped success/error response identical across application, firmware
and provisioning routes.
