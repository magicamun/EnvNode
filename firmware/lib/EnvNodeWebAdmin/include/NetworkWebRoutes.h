// EnvNode: see repository LICENSE.
#pragma once
#include "IWebRoutes.h"
#include "IWebPagePresenter.h"
#include "INetworkSettings.h"
#include "INetworkSaveObserver.h"
#include "IWiFiService.h"
#include "WebHost.h"
#include "WebLocalization.h"
namespace EnvNode {
class NetworkWebRoutes : public IWebRoutes {
public:
    NetworkWebRoutes(WebHost& host, INetworkSettings& settings, IWiFiService& wifi,
        IWebPagePresenter& pages, INetworkSaveObserver& observer);
    void registerRoutes(WebServer& server) override;
private:
    void handleNetwork();
    void handleNetworkSave();
    const char* uiText(TextId id) const;
    WebHost& host_;
    INetworkSettings& settings_;
    IWiFiService& wifiService_;
    IWebPagePresenter& pages_;
    INetworkSaveObserver& observer_;
};
}
