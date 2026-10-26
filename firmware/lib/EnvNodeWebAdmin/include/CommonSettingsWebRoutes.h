// EnvNode: see repository LICENSE.
#pragma once
#include <functional>
#include "WebHost.h"
#include "IWebPagePresenter.h"
#include "IWebPageHeaderPresenter.h"
#include "IBaseSettings.h"
#include "ITimeService.h"
#include "LocaleFormatter.h"
#include "WebLocalization.h"
namespace EnvNode {
class CommonSettingsWebRoutes : public IWebRoutes {
public:
    using DeviceExtras = std::function<String()>;
    using TimeSaved = std::function<void(bool, bool, bool)>;
    using DeviceSaved = std::function<void(bool)>;
    CommonSettingsWebRoutes(WebHost& host, IWebPagePresenter& pages,
        IWebPageHeaderPresenter& headers, IBaseSettings& settings, ITimeService& time,
        LocaleFormatter& formatter, DeviceExtras extras, TimeSaved timeSaved, DeviceSaved deviceSaved);
    void registerRoutes(WebServer& server) override;
private:
    const char* uiText(TextId id) const;
    void sendPage(const char* title, const char* route, const String& content);
    String currentLocalDateTime() const;
    void handleTime();
    void handleTimeSave();
    void handleDevice();
    void handleDeviceSave();
    WebHost& host_;
    WebServer& server_;
    IWebPagePresenter& pages_;
    IWebPageHeaderPresenter& headers_;
    IBaseSettings& settings_;
    ITimeService& timeService_;
    LocaleFormatter& localeFormatter_;
    DeviceExtras deviceExtras_;
    TimeSaved timeSaved_;
    DeviceSaved deviceSaved_;
};
}
