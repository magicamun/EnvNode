// EnvNode: see repository LICENSE.
#include "LogWebRoutes.h"
#include "LogWebView.h"
#include "WebLocalization.h"
namespace EnvNode {
LogWebRoutes::LogWebRoutes(WebHost& host, IWebPagePresenter& pages, const IRecentLogReader& reader)
    : host_(host), pages_(pages), logReader_(reader) {}
void LogWebRoutes::registerRoutes(WebServer& server) {
    server.on("/logs", HTTP_GET, [this]() { handleLogs(); });
    server.on("/logs/data", HTTP_GET, [this]() { handleLogData(); });
}
void LogWebRoutes::handleLogs() {
    pages_.sendPage(webText(pages_.webLocale(), TextId::Logs), "/logs", buildRecentLogHtml(logReader_, pages_.webLocale()));
}
void LogWebRoutes::handleLogData() {
    if (!host_.administrationAvailable()) {
        host_.server().send(503, "application/json", "{\"error\":\"Administration unavailable\"}");
        return;
    }
    host_.server().send(200, "application/json; charset=utf-8", buildRecentLogJson(logReader_));
}
}
