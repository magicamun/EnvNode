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
#include "testing/TestWebRoutes.h"
#include "testing/FixtureRouting.h"
#include "provisioning/ProvisioningI2C.h"
#include "IdentityEeprom24LC32.h"
#include "HardwareProvisioningWebRoutes.h"
#include "provisioning/ProvisioningPage.h"
#include "profiles/HttpsProfileTransport.h"
#include "diagnostics/HardwareDiagnostics.h"
#include "diagnostics/Esp32FixtureIo.h"
#include "testing/PhysicalFixture.h"
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
    TestBoardApplication(ILogger& logger, Testing::TestRunner& tests, Testing::ProfileRepository& profiles, IWiFiService& wifi, ITimeService& time, std::function<bool()> hardwareBusy)
        : logger_(logger), tests_(tests), profiles_(profiles), wifi_(wifi), time_(time), hardwareBusy_(hardwareBusy) {}
    // No GPIO or I²C activity at boot; provisioning connects per operation.
    void initializeDevices() override {}
    void beginIntegrations() override {
        logger_.infof("Testboard firmware version: %s", FirmwareBuildInfo::SemanticVersion);
        logger_.infof("Build: %s", FirmwareBuildInfo::BuildNumber);
        logger_.infof("Git commit: %s", FirmwareBuildInfo::GitCommit);
        logger_.info("Testboard base ready; physical connector measurement ready (explicit start required); EEPROM provisioning ready on SDA16/SCL5");
    }
    void serviceMeasurements() override {
        if(tests_.phase()==Testing::Phase::Running&&!wifi_.connected()&&!wifi_.inSetupAccessPointMode())tests_.abort();
        tests_.tick(millis());
    }
    void serviceIntegrations() override { profiles_.service(wifi_.connected() && time_.synchronized(), tests_.phase()==Testing::Phase::Running || hardwareBusy_()); }
private:
    ILogger& logger_;
    Testing::TestRunner& tests_;
    Testing::ProfileRepository& profiles_;
    IWiFiService& wifi_;
    ITimeService& time_;
    std::function<bool()> hardwareBusy_;
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
Esp32FixtureIo measurementIo;
Testing::PhysicalFixture physicalFixture(measurementIo, []() { return millis(); });
Testing::SelectableFixture measurement(fixture, &physicalFixture, false);
Testing::TestRunner tests(measurement);
Testing::TestWebRoutes testRoutes(host, web, tests, fixture);
HardwareDiagnostics diagnostics(host, web, []() { return tests.phase() != Testing::Phase::Running; }, measurementIo);
ProvisioningI2C provisioningBus(Testing::fixtureRevision01().provisioningSda,
    Testing::fixtureRevision01().provisioningScl);
IdentityEeprom24LC32 targetEeprom(provisioningBus, 0x50);
BoardIdentityStore targetStore(targetEeprom);
TestBoardProvisioning provisioning(provisioningBus, targetStore,
    []() { return !diagnostics.active() && tests.phase() != Testing::Phase::Running; });
ProvisioningWebOptions provisioningOptions() {
    ProvisioningWebOptions options;
    options.externalTarget = true;
    options.modules = false;
    options.available = []() { return provisioning.available(); };
    return options;
}
HardwareProvisioningWebRoutes provisioningRoutes(host.server(), web, logger, provisioning,
    []() { return buildProvisioningOverview(provisioning, web.webLocale()); },
    [](uint8_t*, size_t) {}, provisioningOptions());
Testing::HttpsProfileTransport profileTransport(logger);
Testing::ProfileRepository profiles(profileTransport);
TestBoardApplication application(logger, tests, profiles, wifi, time, []() { return diagnostics.active() || host.responsePending(); });
FirmwareRuntime runtime(wifi, time, web, application);
}
void setupTestBoard() {
    logger.begin(115200);
    delay(500);
    if (!settings.load()) logger.error("Testboard settings unavailable; using defaults");
    testRoutes.attachProfiles(profiles);
    testRoutes.attachMeasurement(measurement);
    testRoutes.setHardwareBusy([]() { return diagnostics.active(); });
    web.attachTestRoutes(testRoutes);
    web.attachDiagnosticsRoutes(diagnostics);
    web.attachProvisioningRoutes(provisioningRoutes);
    runtime.begin();
}
void loopTestBoard() { diagnostics.tick(millis()); runtime.loop(); }
}
void setup() { EnvNode::setupTestBoard(); }
void loop() { EnvNode::loopTestBoard(); }
