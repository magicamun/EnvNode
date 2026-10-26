// EnvNode: see repository LICENSE.
#pragma once
#include <functional>
#include "IWebRoutes.h"
#include "IWebPagePresenter.h"
#include "OTAService.h"
#include "WebLocalization.h"
namespace EnvNode {
struct FirmwareWebBuildInfo {
    const char* SemanticVersion;
    const char* CompactIdentity;
    const char* BuildNumber;
    const char* GitCommit;
    const char* GitBranch;
    const char* BuildTimestampUtc;
    const char* SourceState;
    bool SourceDirty;
};
class FirmwareWebRoutes : public IWebRoutes {
public:
    using ResetConfiguration = std::function<bool()>;
    using FormatNumber = std::function<String(double, uint8_t)>;
    FirmwareWebRoutes(WebServer& server, IWebPagePresenter& pages, OTAService& ota,
        IDeviceRestart& restart, FirmwareWebBuildInfo build,
        ResetConfiguration resetConfiguration, FormatNumber formatNumber);
    void registerRoutes(WebServer& server) override;
private:
    const char* uiText(TextId id) const;
    void sendPage(const char* title, const char* route, const String& content, int status=200);
    void sendResult(const char* title, const char* route, const char* message, bool success);
    String otaStatusHtml() const;
    void handleFirmware();
    void handleFirmwareUploadData();
    void handleFirmwareUpload();
    void handleRestart();
    void handleFactoryReset();
    void performExplicitRestart();
    WebServer& server_;
    IWebPagePresenter& pages_;
    OTAService& otaService_;
    IDeviceRestart& deviceRestart_;
    FirmwareWebBuildInfo build_;
    ResetConfiguration resetConfiguration_;
    FormatNumber formatNumber_;
    bool firmwareUploadRequestAccepted_ = false;
    String firmwareUploadRequestError_;
};
}
