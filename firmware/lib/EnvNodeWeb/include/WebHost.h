// EnvNode: see repository LICENSE.
#pragma once
#include <WebServer.h>
#include <functional>
#include "IWebRoutes.h"
#include "IWiFiService.h"
#include "Logger.h"
namespace EnvNode {
class WebHost {
public:
    WebHost(ILogger& logger, IWiFiService& wifi, uint16_t port = 80);
    WebServer& server();
    void registerRoutes(IWebRoutes& routes);
    void begin();
    void loop();
    bool administrationAvailable() const;
    void sendPage(const std::function<String()>& renderHeader, const String& content,
        int status, const char* unavailableMessage, const char* renderFailureMessage);
private:
    ILogger& logger_;
    IWiFiService& wifi_;
    WebServer server_;
};
}
