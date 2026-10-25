# EnvNodeWeb

Shared synchronous HTTP host for EnvNode and future fixture applications.

- WebHost owns WebServer and services its clients. Applications register one or more IWebRoutes providers before begin(). Providers and captured handler state must outlive the server.
- WebHost serves the existing /style.css asset with its previous cache policy, and streams page header, application content and footer without constructing a second complete-page String. It preserves 503 responses while station WiFi is connecting and permits access in setup-AP mode.
- WebPageFrame accepts product/device/version metadata, language, setup-mode text and application-supplied script/navigation/notice renderers. Fragments render sequentially to avoid keeping all temporary fragment Strings alive alongside the page header. Renderers run synchronously; referenced context must remain valid for the call.
- Text fields are HTML-escaped. Script, navigation and notices are trusted HTML fragments produced by application code, not direct request parameters.
- HtmlEscaping, HttpRedirect and IWebService retain their original include names.

This library has no sensor, actuator, controller, EEPROM, OTA or application Configuration dependencies. EnvNode WebService remains the application route provider. Its existing pages, navigation and translation catalog remain application-owned. The stylesheet retains existing application selectors to preserve the current appearance.

WebHost exposes the HTTP server to route providers; it is not a new asynchronous server or WebSocket implementation. Separating reusable network/log/OTA pages is a later step.

EnvNode-owned files follow the repository LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
