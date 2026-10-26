// EnvNode: see repository LICENSE.
#include <Arduino.h>
#include <ctime>
#include "FirmwareBuildInfo.h"
#include "TestBoardSettings.h"
#include "TestBoardRestart.h"
#include "TestBoardWeb.h"
#include "FirmwareRuntime.h"
#include "StructuredLogger.h"
#include "SerialLogger.h"
#include "WiFiService.h"
#include "TimeService.h"
namespace EnvNode {
namespace {
class TestBoardLogTime : public ILogTimeProvider {
public:
    uint32_t monotonicMs() const override { return millis(); }
    bool wallClockEpochSeconds(int64_t& epoch) const override {
        epoch = static_cast<int64_t>(std::time(nullptr));
        return epoch > 1609459200LL;
    }
};
class TestBoardApplication : public IFirmwareApplication {
public:
    explicit TestBoardApplication(ILogger& logger) : logger_(logger) {}
    // Hardware is not driven until the fixture test engine is implemented.
    void initializeDevices() override {}
    void beginIntegrations() override {
        logger_.infof("Testboard firmware version: %s", FirmwareBuildInfo::SemanticVersion);
        logger_.infof("Build: %s", FirmwareBuildInfo::BuildNumber);
        logger_.infof("Git commit: %s", FirmwareBuildInfo::GitCommit);
        logger_.info("Testboard base ready; measurement and provisioning inactive");
    }
    void serviceMeasurements() override {}
    void serviceIntegrations() override {}
private:
    ILogger& logger_;
};
TestBoardLogTime logTime;
RecentLogStore recentLogs;
SerialLogger serialSink;
StructuredLogger logger(recentLogs, logTime, serialSink);
TestBoardSettings settings;
WiFiService wifi(logger, settings.base());
TimeService time(logger, settings.base(), wifi);
LocaleFormatter formatter(settings.base());
TestBoardRestart restart(logger);
OTAService ota(logger, restart);
WebHost host(logger, wifi);
TestBoardWeb web(host, settings, wifi, time, formatter, ota, restart, recentLogs,
    {FirmwareBuildInfo::SemanticVersion, FirmwareBuildInfo::CompactIdentity,
     FirmwareBuildInfo::BuildNumber, FirmwareBuildInfo::GitCommit, FirmwareBuildInfo::GitBranch,
     FirmwareBuildInfo::BuildTimestampUtc, FirmwareBuildInfo::SourceState, FirmwareBuildInfo::SourceDirty});
TestBoardApplication application(logger);
FirmwareRuntime runtime(wifi, time, web, application);
}
void setupTestBoard() {
    logger.begin(115200);
    delay(500);
    if (!settings.load()) logger.error("Testboard settings unavailable; using defaults");
    runtime.begin();
}
void loopTestBoard() { runtime.loop(); }
}
void setup() { EnvNode::setupTestBoard(); }
void loop() { EnvNode::loopTestBoard(); }
