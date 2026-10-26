// EnvNode: see repository LICENSE.
#pragma once
#include <functional>
#include "IWebRoutes.h"
#include "IWebPagePresenter.h"
#include "IProvisioningAdministration.h"
#include "WebLocalization.h"
#include "Logger.h"
namespace EnvNode {
class HardwareProvisioningWebRoutes : public IWebRoutes {
public:
    using Overview = std::function<String()>;
    using Random = std::function<void(uint8_t*, size_t)>;
    HardwareProvisioningWebRoutes(WebServer& server, IWebPagePresenter& pages,
        ILogger& logger, IProvisioningAdministration& administration,
        Overview overview, Random random);
    void registerRoutes(WebServer& server) override;
private:
    const char* uiText(TextId id) const;
    void sendPage(const char* title, const char* route, const String& content, int status=200);
    void sendResult(const char* title, const char* route, const char* message, bool success);
    void handleHardware();
    void handleBoardProvisioning();
    void handleModuleDescriptorProvisioning();
    WebServer& server_;
    IWebPagePresenter& pages_;
    ILogger& logger_;
    IProvisioningAdministration& administration_;
    Overview overview_;
    Random random_;
};
}
