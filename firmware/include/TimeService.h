#pragma once

#include <Arduino.h>
#include "ITimeService.h"
#include "IConfigurationService.h"
#include "IWiFiService.h"
#include "Logger.h"

namespace EnvNode {

class TimeService : public ITimeService {
public:
    TimeService(ILogger& logger, IConfigurationService& configurationService, IWiFiService& wifiService);

    void begin() override;
    void loop() override;
    bool synchronized() const override;
    time_t now() const override;
    bool localCivilTime(tm& localTime) const override;
    String iso8601Utc() const override;
    String iso8601Local() const override;
    String iso8601Local(time_t timestamp) const override;
    uint32_t epoch() const override;

private:
    void startSynchronization();
    bool isTimeValid() const;
    String formatIso8601(time_t timestamp, bool local) const;

    ILogger& logger_;
    IConfigurationService& configurationService_;
    IWiFiService& wifiService_;
    bool syncStarted_ = false;
    bool synchronized_ = false;
};

} // namespace EnvNode
