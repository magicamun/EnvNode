// EnvNode: see repository LICENSE.
#pragma once
#include "TestBoardSettings.h"
#include "IWebService.h"
#include "IWebPagePresenter.h"
#include "IWebPageHeaderPresenter.h"
#include "INetworkSaveObserver.h"
#include "NetworkWebRoutes.h"
#include "CommonSettingsWebRoutes.h"
#include "FirmwareWebRoutes.h"
#include "LogWebRoutes.h"
namespace EnvNode {
class TestBoardWeb : public IWebService, public IWebPagePresenter,
    public IWebPageHeaderPresenter, public INetworkSaveObserver {
public:
    TestBoardWeb(WebHost& host, TestBoardSettings& settings, IWiFiService& wifi,
        ITimeService& time, LocaleFormatter& formatter, OTAService& ota,
        IDeviceRestart& restart, const IRecentLogReader& logs, FirmwareWebBuildInfo build);
    void begin() override;
    void loop() override;
    Locale webLocale() const override;
    void sendPage(const char* title, const char* activeRoute, const String& content,
        int status = 200, bool wideContent = false) override;
    String renderPageHeader(const char* title, const char* active, bool wideContent) const override;
    void networkSettingsSaved(bool success) override;
private:
    const char* text(TextId id) const;
    const char* testerText(const char* german, const char* english) const;
    String navigation(const char* active) const;
    String notices() const;
    void home();
    void settingsResult(bool ok, TextId success, TextId failure, const char* route, bool restart);
    TestBoardSettings& settings_;
    IWiFiService& wifi_;
    IDeviceRestart& restart_;
    FirmwareWebBuildInfo build_;
    WebHost& host_;
    NetworkWebRoutes networkRoutes_;
    CommonSettingsWebRoutes commonRoutes_;
    FirmwareWebRoutes firmwareRoutes_;
    LogWebRoutes logRoutes_;
};
}
