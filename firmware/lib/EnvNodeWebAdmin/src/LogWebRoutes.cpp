// EnvNode: see repository LICENSE.
#include "LogWebRoutes.h"
#include "LogWebView.h"
#include "WebLocalization.h"
namespace EnvNode {
namespace {
bool pageNumber(WebServer& server,const char* key,size_t fallback,size_t maximum,size_t& value) {
    value=fallback;if(!server.hasArg(key))return true;
    const String text=server.arg(key);if(text.isEmpty() || text.length()>2)return false;
    value=0;for(size_t i=0;i<text.length();++i) {
        if(text[i]<'0'||text[i]>'9')return false;
        value=value*10+static_cast<size_t>(text[i]-'0');
    }
    return value<=maximum;
}
}
LogWebRoutes::LogWebRoutes(WebHost& host, IWebPagePresenter& pages, const IRecentLogReader& reader)
    : host_(host), pages_(pages), logReader_(reader) {}
void LogWebRoutes::registerRoutes(WebServer& server) {
    server.on("/log-view.js", HTTP_GET, [this]() {
        host_.sendStaticContent("application/javascript; charset=utf-8",recentLogScript());
    });
    server.on("/logs", HTTP_GET, [this]() { handleLogs(); });
    server.on("/logs/data", HTTP_GET, [this]() { handleLogData(); });
}
void LogWebRoutes::handleLogs() {
    size_t offset=0;
    if(!pageNumber(host_.server(),"offset",0,64,offset)) {host_.server().send(400,"text/plain","Invalid log offset");return;}
    pages_.sendPage(webText(pages_.webLocale(), TextId::Logs), "/logs", buildRecentLogHtml(logReader_, pages_.webLocale(),offset,8));
}
void LogWebRoutes::handleLogData() {
    if (!host_.administrationAvailable()) {
        host_.server().send(503, "application/json", "{\"error\":\"Administration unavailable\"}");
        return;
    }
    size_t offset=0,limit=64;
    if(!pageNumber(host_.server(),"offset",0,64,offset) || !pageNumber(host_.server(),"limit",64,8,limit) || !limit) {
        host_.server().send(400,"application/json","{\"error\":\"Invalid log page\"}");return;
    }
    const String json=buildRecentLogJson(logReader_,offset,limit);
    if(json.isEmpty()) {host_.server().send(503,"application/json","{\"error\":\"Insufficient memory\"}");return;}
    host_.server().send(200, "application/json; charset=utf-8",json);
}
}
