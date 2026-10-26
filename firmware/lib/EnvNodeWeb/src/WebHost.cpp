// EnvNode: see repository LICENSE.
#include "WebHost.h"
#include "WebPageFrame.h"
#include "WebStyle.h"
#include <cstring>
namespace EnvNode {
WebHost::WebHost(ILogger& logger, IWiFiService& wifi, uint16_t port)
    : logger_(logger), wifi_(wifi), server_(port) {}
WebServer& WebHost::server() { return server_; }
void WebHost::registerRoutes(IWebRoutes& routes) { routes.registerRoutes(server_); }
void WebHost::begin() {
    server_.on("/style.css", HTTP_GET, [this]() {
        const size_t length = sharedWebStyleLength();
        server_.sendHeader("Cache-Control", "public, max-age=86400");
        server_.send_P(200, PSTR("text/css"), sharedWebStyle(), length);
    });
    server_.begin();
    logger_.info("Web administration started");
}
void WebHost::loop() { server_.handleClient(); }
bool WebHost::administrationAvailable() const {
    return wifi_.inSetupAccessPointMode() || wifi_.connected();
}
void WebHost::sendPage(const std::function<String()>& renderHeader,
    const String& content, int status, const char* unavailableMessage,
    const char* renderFailureMessage) {
    if (!administrationAvailable()) {
        server_.send(503, "text/plain", unavailableMessage);
        return;
    }
    const String header = renderHeader();
    const char* footer = webPageFooter();
    if (header.isEmpty() || content.isEmpty()) {
        logger_.errorf("Web page rendering failed: header=%u content=%u heap=%u largest=%u",
            static_cast<unsigned>(header.length()), static_cast<unsigned>(content.length()),
            static_cast<unsigned>(ESP.getFreeHeap()), static_cast<unsigned>(ESP.getMaxAllocHeap()));
        server_.send(503, "text/plain", renderFailureMessage);
        return;
    }
    server_.setContentLength(header.length() + content.length() + strlen(footer));
    server_.send(status, "text/html; charset=utf-8", header);
    server_.sendContent(content);
    server_.sendContent(footer, strlen(footer));
}
}
