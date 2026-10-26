// EnvNode: see repository LICENSE.
#include "TestBoardWeb.h"
#include "WebPageFrame.h"
#include "WebResult.h"
#include "HtmlEscaping.h"
namespace EnvNode {
TestBoardWeb::TestBoardWeb(WebHost& host, TestBoardSettings& settings, IWiFiService& wifi,
    ITimeService& time, LocaleFormatter& formatter, OTAService& ota,
    IDeviceRestart& restart, const IRecentLogReader& logs, FirmwareWebBuildInfo build)
    : settings_(settings), wifi_(wifi), restart_(restart), build_(build), host_(host),
      networkRoutes_(host_, settings.base(), wifi, *this, *this),
      commonRoutes_(host_, *this, *this, settings.base(), time, formatter,
          []() { return String(); },
          [this](bool ok, bool, bool timeChanged) {
              settingsResult(ok, TextId::LocaleAndTimeSaved, TextId::LocaleAndTimeSaveFailed,
                  "/time", timeChanged);
          },
          [this](bool ok) {
              settingsResult(ok, TextId::DeviceSettingsSaved, TextId::DeviceSaveFailed, "/device", false);
          }),
      firmwareRoutes_(host_.server(), *this, ota, restart, build,
          [this]() { return settings_.reset(); },
          [&formatter](double value, uint8_t digits) { return formatter.formatNumber(value, digits); }),
      logRoutes_(host_, *this, logs) {}
void TestBoardWeb::begin() {
    host_.server().on("/", HTTP_GET, [this]() { home(); });
    host_.registerRoutes(networkRoutes_);
    host_.registerRoutes(commonRoutes_);
    host_.registerRoutes(firmwareRoutes_);
    host_.registerRoutes(logRoutes_);
    host_.begin();
}
void TestBoardWeb::loop() { host_.loop(); }
Locale TestBoardWeb::webLocale() const { return settings_.base().getLocale(); }
const char* TestBoardWeb::text(TextId id) const { return webText(webLocale(), id); }
const char* TestBoardWeb::testerText(const char* german, const char* english) const {
    return webLocale() == Locale::GermanGermany ? german : english;
}
String TestBoardWeb::navigation(const char* active) const {
    String html("<nav class='nav'>");
    const auto link = [&](const char* route, const char* label) {
        html += String("<a href='") + route + "'";
        if (String(active) == route) html += " class='active'";
        html += String(">") + escapeHtml(label) + "</a>";
    };
    link("/", testerText("Prüfboard", "Test board"));
    link("/network", text(TextId::Network));
    link("/time", text(TextId::LocaleTime));
    link("/device", text(TextId::Device));
    link("/logs", text(TextId::Logs));
    link("/firmware", text(TextId::Firmware));
    html += "</nav>";
    return html;
}
String TestBoardWeb::notices() const {
    if (!restart_.deviceRestartPending()) return String();
    return String("<div class='notice'>") + text(TextId::RestartRequired)
        + "<form method='post' action='/restart'><button>" + text(TextId::RestartNow)
        + "</button></form></div>";
}
String TestBoardWeb::renderPageHeader(const char* title, const char* active, bool wide) const {
    WebPageFrameData page{title, settings_.base().getDeviceConfiguration().name,
        "EnvNode Testboard", build_.SemanticVersion, build_.BuildTimestampUtc,
        webLanguageTag(webLocale()), [this]() { return buildWebUiScript(webLocale()); },
        [this, active]() { return navigation(active); }, [this]() { return notices(); },
        wifi_.inSetupAccessPointMode(), text(TextId::SetupAccessPointMode), wide};
    return buildWebPageHeader(page);
}
void TestBoardWeb::sendPage(const char* title, const char* active, const String& content,
    int status, bool wide) {
    host_.sendPage([this, title, active, wide]() { return renderPageHeader(title, active, wide); },
        content, status, text(TextId::AdministrationUnavailableWhileNetworkIsConnecting),
        text(TextId::PageRenderingFailedPleaseRetry));
}
void TestBoardWeb::home() {
    String content("<section class='card'><h2>");
    content += testerText("Prüfboard-Basis", "Test board base");
    content += "</h2><p>";
    content += testerText("Netzwerk, Geräteeinstellungen, Sprache/Zeit, Protokolle und Firmwareverwaltung sind verfügbar.",
        "Network, device settings, locale/time, logs and firmware administration are available.");
    content += "</p><p>";
    content += testerText("Anschlussprüfungen und EEPROM-Provisioning sind noch nicht implementiert.",
        "Connector tests and EEPROM provisioning are not implemented yet.");
    content += "</p></section>";
    sendPage(testerText("Prüfboard", "Test board"), "/", content);
}
void TestBoardWeb::settingsResult(bool ok, TextId success, TextId failure,
    const char* route, bool restart) {
    if (ok && restart) restart_.requestDeviceRestart();
    sendWebResult(*this, text(ok ? success : failure), route,
        text(ok ? (restart ? TextId::ConfigurationSavedDeviceRestartRequired
                          : TextId::ConfigurationSavedChangesAreActive)
                : failure), ok);
}
void TestBoardWeb::networkSettingsSaved(bool success) {
    settingsResult(success, TextId::NetworkSettingsSaved, TextId::NetworkSaveFailed, "/network", true);
}
}
