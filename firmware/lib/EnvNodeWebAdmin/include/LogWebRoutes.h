// EnvNode: see repository LICENSE.
#pragma once
#include "IWebRoutes.h"
#include "IWebPagePresenter.h"
#include "IRecentLogReader.h"
#include "WebHost.h"
namespace EnvNode {
class LogWebRoutes : public IWebRoutes {
public:
    LogWebRoutes(WebHost& host, IWebPagePresenter& pages, const IRecentLogReader& reader);
    void registerRoutes(WebServer& server) override;
private:
    void handleLogs();
    void handleLogData();
    WebHost& host_;
    IWebPagePresenter& pages_;
    const IRecentLogReader& logReader_;
};
}
