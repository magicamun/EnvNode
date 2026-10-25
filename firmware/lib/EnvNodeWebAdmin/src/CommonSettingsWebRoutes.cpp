// EnvNode: see repository LICENSE.
#include "CommonSettingsWebRoutes.h"
#include "HtmlEscaping.h"
namespace EnvNode {
namespace {
struct TimezoneOption { const char* label; const char* value; };
const TimezoneOption TimezoneOptions[] = {
#include "TimezoneOptions.inc"
};

String badge(const char* text, const char* style) { return "<span class='badge " + String(style) + "'>" + text + "</span>"; }
void sendTimezoneSelect(WebServer& server, const String& current, Locale locale) {
    // Keep the worldwide catalog out of contiguous page-sized heap allocations.
    String chunk;
    chunk.reserve(1200);
    chunk = "<select name='timezone'>";
    bool found = false;
    for (const auto& option : TimezoneOptions) {
        const bool selected = !found && current == option.value;
        found = found || selected;
        chunk += "<option value='" + escapeHtml(option.value) + "'" + (selected ? " selected" : "") + ">" + option.label + "</option>";
        if (chunk.length() >= 1024) {
            server.sendContent(chunk);
            chunk = "";
        }
    }
    if (!found && !current.isEmpty()) chunk += "<option value='" + escapeHtml(current) + (String("' selected>") + webText(locale, TextId::Custom) + " ") + escapeHtml(current) + "</option>";
    chunk += "</select>";
    server.sendContent(chunk);
}
String localeSelect(Locale current) {
    const Locale supported[] = {
        Locale::GermanGermany,
        Locale::EnglishUnitedKingdom,
        Locale::EnglishUnitedStates,
    };
    String html;
    html.reserve(300);
    html = "<select name='locale'>";
    for (const Locale locale : supported) {
        const char* key = localeKey(locale);
        html += "<option value='";
        html += key;
        html += "'";
        if (locale == current) html += " selected";
        html += ">";
        switch (locale) {
            case Locale::GermanGermany: html += "Deutsch (de-DE)"; break;
            case Locale::EnglishUnitedKingdom: html += "English (en-GB)"; break;
            case Locale::EnglishUnitedStates: html += "English (en-US)"; break;
        }
        html += "</option>";
    }
    html += "</select>";
    return html;
}
}
CommonSettingsWebRoutes::CommonSettingsWebRoutes(WebHost& host, IWebPagePresenter& pages,
    IWebPageHeaderPresenter& headers, IBaseSettings& settings, ITimeService& time,
    LocaleFormatter& formatter, DeviceExtras extras, TimeSaved timeSaved, DeviceSaved deviceSaved)
    : host_(host), server_(host.server()), pages_(pages), headers_(headers), settings_(settings),
      timeService_(time), localeFormatter_(formatter), deviceExtras_(extras),
      timeSaved_(timeSaved), deviceSaved_(deviceSaved) {}
void CommonSettingsWebRoutes::registerRoutes(WebServer& server) {
    server.on("/time", HTTP_GET, [this]() { handleTime(); });
    server.on("/time/save", HTTP_POST, [this]() { handleTimeSave(); });
    server.on("/device", HTTP_GET, [this]() { handleDevice(); });
    server.on("/device/save", HTTP_POST, [this]() { handleDeviceSave(); });
}
const char* CommonSettingsWebRoutes::uiText(TextId id) const { return webText(pages_.webLocale(), id); }
void CommonSettingsWebRoutes::sendPage(const char* title, const char* route, const String& content) { pages_.sendPage(title, route, content); }
String CommonSettingsWebRoutes::currentLocalDateTime() const {
    tm localTime;
    return timeService_.localCivilTime(localTime)
        ? localeFormatter_.formatDateTime(localTime)
        : String("—");
}
void CommonSettingsWebRoutes::handleTime() {
    if (!host_.administrationAvailable()) {
        server_.send(503, "text/plain", uiText(TextId::AdministrationUnavailableWhileNetworkIsConnecting));
        return;
    }
    const String header = headers_.renderPageHeader(uiText(TextId::LocaleTime), "/time", false);
    if (header.isEmpty()) {
        server_.send(503, "text/plain", uiText(TextId::PageRenderingFailedPleaseRetry));
        return;
    }
    const TimeConfiguration& t = settings_.getTimeConfiguration();
    const Locale currentLocale = settings_.getLocale();
    String c;
    c.reserve(1300);
    c = (String("<section class='card'><h2>") + uiText(TextId::RuntimeStatus) + "</h2><div class='kv'><span>" + uiText(TextId::Synchronization) + "</span><span>") + badge(uiText(timeService_.synchronized()?TextId::Synchronized:TextId::Synchronizing),timeService_.synchronized()?"good":"warn") + (String("</span><span>") + uiText(TextId::LocalTime) + "</span><span>") + (timeService_.synchronized()?escapeHtml(currentLocalDateTime()):"—") + (String("</span></div></section><form method='post' action='/time/save'><section class='card'><h2>") + uiText(TextId::Locale) + "</h2><label>" + uiText(TextId::Locale) + "") + localeSelect(currentLocale) + (String("</label></section><section class='card'><h2>") + uiText(TextId::Time) + "</h2>");
    c += String("<label>") + uiText(TextId::Timezone);
    server_.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server_.send(200, "text/html; charset=utf-8", header);
    server_.sendContent(c);
    c = "";
    sendTimezoneSelect(server_, t.timezone, currentLocale);
    c = (String("</label><label>") + uiText(TextId::NTPServer1) + "<input name='ntpServer1' value='") + escapeHtml(t.ntpServer1) + (String("'></label><label>") + uiText(TextId::NTPServer2) + "<input name='ntpServer2' value='") + escapeHtml(t.ntpServer2) + (String("'></label><div class='actions'><button>") + uiText(TextId::SaveLocaleAndTime) + "</button></div></section></form>");
    server_.sendContent(c);
    server_.sendContent("</main></div></body></html>");
    server_.sendContent("");
}
void CommonSettingsWebRoutes::handleDevice() {
    String c;
    c = (((String("<section class='card'><h2>") + uiText(TextId::DeviceIdentity) + "</h2><form method='post' action='/device/save'><label>")) + uiText(TextId::DeviceName) + "<input name='deviceName' required value='") + escapeHtml(settings_.getDeviceConfiguration().name) + (String("'></label><div class='actions'><button>") + uiText(TextId::SaveDeviceSettings) + "</button></div></form></section>");
    c += deviceExtras_();
    sendPage(uiText(TextId::Device), "/device", c);
}
void CommonSettingsWebRoutes::handleTimeSave() {
    const TimeConfiguration& current = settings_.getTimeConfiguration();
    const Locale previousLocale = settings_.getLocale();
    Locale locale;
    bool ok = parseLocaleKey(server_.arg("locale").c_str(), locale);
    const bool localeChanged = ok && locale != previousLocale;
    const bool timeChanged = server_.arg("timezone") != current.timezone
        || server_.arg("ntpServer1") != current.ntpServer1
        || server_.arg("ntpServer2") != current.ntpServer2;
    if (ok) {
        ok = settings_.setLocale(locale)
            && settings_.setTimezone(server_.arg("timezone"))
            && settings_.setNtpServer1(server_.arg("ntpServer1"))
            && settings_.setNtpServer2(server_.arg("ntpServer2"));
    }
    timeSaved_(ok, localeChanged, timeChanged);
}
void CommonSettingsWebRoutes::handleDeviceSave() { const bool ok=settings_.setDeviceName(server_.arg("deviceName")); deviceSaved_(ok); }
}
