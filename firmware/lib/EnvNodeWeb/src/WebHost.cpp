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
void WebHost::loop() {
    server_.beginMeasurement();headerRenderMs_=0;
    const uint32_t started=millis();
    server_.handleClient();
    const uint32_t elapsed=millis()-started;
    if(elapsed>=1000) logger_.warnf(
        "Web request slow: elapsed_ms=%lu route=%s first_write_ms=%lu send_ms=%lu header_ms=%lu bytes=%u heap=%u largest=%u",
        static_cast<unsigned long>(elapsed),server_.responseRoute().isEmpty() ? "(no response)" : server_.responseRoute().c_str(),
        static_cast<unsigned long>(server_.firstWriteMs()),static_cast<unsigned long>(server_.writeMs()),
        static_cast<unsigned long>(headerRenderMs_),static_cast<unsigned>(server_.writtenBytes()),
        static_cast<unsigned>(ESP.getFreeHeap()),static_cast<unsigned>(ESP.getMaxAllocHeap()));
}
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
    const uint32_t started=millis();
    const String header = renderHeader();
    headerRenderMs_+=millis()-started;
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
