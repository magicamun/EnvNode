// EnvNode: see repository LICENSE.
#include <Arduino.h>
#include <esp_system.h>
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
#include "testing/TestWebRoutes.h"
#include "testing/FixtureRouting.h"
#include "provisioning/ProvisioningI2C.h"
#include "IdentityEeprom24LC32.h"
#include "HardwareProvisioningWebRoutes.h"
#include "provisioning/ProvisioningPage.h"
#include "profiles/HttpsProfileTransport.h"
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
    TestBoardApplication(ILogger& logger, Testing::TestRunner& tests, Testing::ProfileRepository& profiles, IWiFiService& wifi, ITimeService& time)
        : logger_(logger), tests_(tests), profiles_(profiles), wifi_(wifi), time_(time) {}
    // No GPIO or I²C activity at boot; provisioning connects per operation.
    void initializeDevices() override {}
    void beginIntegrations() override {
        logger_.infof("Testboard firmware version: %s", FirmwareBuildInfo::SemanticVersion);
        logger_.infof("Build: %s", FirmwareBuildInfo::BuildNumber);
        logger_.infof("Git commit: %s", FirmwareBuildInfo::GitCommit);
        logger_.info("Testboard base ready; simulated connector tests ready; measurement I/O inactive; EEPROM provisioning ready on SDA16/SCL5");
    }
    void serviceMeasurements() override { tests_.tick(millis()); }
    void serviceIntegrations() override { profiles_.service(wifi_.connected() && time_.synchronized(), tests_.phase()==Testing::Phase::Running); }
private:
    ILogger& logger_;
    Testing::TestRunner& tests_;
    Testing::ProfileRepository& profiles_;
    IWiFiService& wifi_;
    ITimeService& time_;
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
Testing::SimulatedFixture fixture;
Testing::TestRunner tests(fixture);
Testing::TestWebRoutes testRoutes(host, web, tests, fixture);
ProvisioningI2C provisioningBus(Testing::fixtureRevision01().provisioningSda,
    Testing::fixtureRevision01().provisioningScl);
IdentityEeprom24LC32 targetEeprom(provisioningBus, 0x50);
BoardIdentityStore targetStore(targetEeprom);
BoardPortMapStore targetPortMaps(targetEeprom);
IdentityEeprom24LC32 moduleAEeprom(provisioningBus, 0x52), moduleBEeprom(provisioningBus, 0x53);
HardwareDescriptorStore moduleAStore(moduleAEeprom), moduleBStore(moduleBEeprom);
TestBoardProvisioning provisioning(provisioningBus, targetStore,
    []() { return tests.phase() != Testing::Phase::Running; }, &moduleAStore, &moduleBStore, &targetPortMaps);
ProvisioningWebOptions provisioningOptions() {
    ProvisioningWebOptions options;
    options.externalTarget = true;
    options.modules = true;
    options.available = []() { return provisioning.available(); };
    return options;
}
HardwareProvisioningWebRoutes provisioningRoutes(host.server(), web, logger, provisioning,
    []() { return buildProvisioningOverview(provisioning, web.webLocale()); },
    [](uint8_t* bytes, size_t size) {
        for (size_t i = 0; i < size; ++i) bytes[i] = static_cast<uint8_t>(esp_random());
    }, provisioningOptions());
Testing::HttpsProfileTransport profileTransport(logger);
Testing::ProfileRepository profiles(profileTransport);
TestBoardApplication application(logger, tests, profiles, wifi, time);
FirmwareRuntime runtime(wifi, time, web, application);
}
void setupTestBoard() {
    logger.begin(115200);
    delay(500);
    if (!settings.load()) logger.error("Testboard settings unavailable; using defaults");
    testRoutes.attachProfiles(profiles);
    web.attachTestRoutes(testRoutes);
    web.attachProvisioningRoutes(provisioningRoutes);
    runtime.begin();
}
void loopTestBoard() { runtime.loop(); }
}
void setup() { EnvNode::setupTestBoard(); }
void loop() { EnvNode::loopTestBoard(); }
