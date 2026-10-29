// EnvNode: see repository LICENSE.
#pragma once
#include <functional>
#include "IWebRoutes.h"
#include "IWebPagePresenter.h"
#include "IProvisioningAdministration.h"
#include "WebLocalization.h"
#include "Logger.h"
#include "HardwareProfileRepository.h"
namespace EnvNode {
struct ProvisioningWebOptions {
    bool externalTarget = false;
    bool modules = true;
    std::function<bool()> available;
};
class HardwareProvisioningWebRoutes : public IWebRoutes {
public:
    using Overview = std::function<String()>;
    using Random = std::function<void(uint8_t*, size_t)>;
    HardwareProvisioningWebRoutes(WebServer& server, IWebPagePresenter& pages,
        ILogger& logger, IProvisioningAdministration& administration,
        Overview overview, Random random, ProvisioningWebOptions options = {});
    void attachProfiles(HardwareProfileRepository& profiles) { profiles_ = &profiles; }
    bool enableRemoteProfiles();
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
    ProvisioningWebOptions options_;
    bool operationAllowed();
    String remoteProfilesSection();
    void handleRemoteProfileWrite();
    void handleProfileRefresh();
    HardwareProfileRepository* profiles_ = nullptr;
    std::unique_ptr<IHardwareProfileTransport> profileTransport_;
    std::unique_ptr<HardwareProfileRepository> ownedProfiles_;
};
}
