// EnvNode: see repository LICENSE.
#include <unity.h>
#include <cstring>
#include "provisioning/TestBoardProvisioning.h"
#include "provisioning/ProvisioningI2C.h"
#include "provisioning/ProvisioningPage.h"
#include "IdentityEeprom24LC32.h"
#include "Update.h"
#include "HardwareProvisioningWebRoutes.h"
#include "testing/TestRunner.h"
#include "testing/SimulatedFixture.h"
using namespace EnvNode;
UpdateClass Update;
int pinModes[40];
void pinMode(unsigned char pin, int mode) { pinModes[pin] = mode; }
HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t n) { return n; }
struct Logger : ILogger {
    void begin(unsigned long) override {}
    void println(const char*) override {}
    void printf(const char*, ...) override {}
};
struct Pages : IWebPagePresenter {
    Locale locale = Locale::EnglishUnitedKingdom;
    int status = 0;
    String content;
    Locale webLocale() const override { return locale; }
    void sendPage(const char*, const char*, const String& c, int s, bool) override { content = c; status = s; }
};
struct Fixture {
    Logger logger; Pages pages; WebServer server{80};
    Testing::SimulatedFixture simulation; Testing::TestRunner tests{simulation};
    ProvisioningI2C bus{16, 5}; IdentityEeprom24LC32 eeprom{bus, 0x50};
    BoardIdentityStore store{eeprom};
    TestBoardProvisioning administration{bus, store, [this]() { return tests.phase() != Testing::Phase::Running; }};
    HardwareProvisioningWebRoutes routes{server, pages, logger, administration,
        [this]() { return buildProvisioningOverview(administration, pages.locale); },
        [](uint8_t*, size_t) {}, options()};
    ProvisioningWebOptions options() {
        ProvisioningWebOptions o; o.modules = false; o.externalTarget = true;
        o.available = [this]() { return administration.available(); }; return o;
    }
    Fixture() { routes.registerRoutes(server); }
    void form(bool confirmed = true) {
        server.arguments = {{"boardProfileId", "2"}, {"serialNumber", "MINI-EXTERNAL"}};
        if (confirmed) server.arguments["confirmProvisioning"] = "1";
    }
    BoardIdentity identity() { return {BoardProfileId::EnvNodeMini, {0, 8}, {"MINI-EXTERNAL"}}; }
};
void setUp() { Wire = TwoWire(); Wire1 = TwoWire(); std::fill_n(pinModes, 40, -1); }
void tearDown() {}
void test_no_bus_at_boot_and_no_module_routes() {
    Fixture f;
    TEST_ASSERT_EQUAL(0, Wire.begins);
    TEST_ASSERT_NULL(f.bus.wire(I2CBus::I2C0));
    TEST_ASSERT_EQUAL(3, f.server.routes.size());
    TEST_ASSERT_FALSE(f.server.request("/hardware/module-descriptor/write", HTTP_POST));
    TEST_ASSERT_FALSE(f.server.request("/device/module-descriptor/write", HTTP_POST));
    TEST_ASSERT_EQUAL(0, Wire.begins);
}
void test_overview_reads_only_external_address_then_releases_bus() {
    Fixture f;
    f.server.request("/hardware", HTTP_GET);
    TEST_ASSERT_EQUAL(200, f.pages.status);
    TEST_ASSERT_EQUAL(1, Wire.begins); TEST_ASSERT_EQUAL(1, Wire.ends);
    TEST_ASSERT_EQUAL(16, Wire.sda); TEST_ASSERT_EQUAL(5, Wire.scl);
    TEST_ASSERT_EQUAL(50, Wire.timeout);
    TEST_ASSERT_NULL(f.bus.wire(I2CBus::I2C0));
    TEST_ASSERT_EQUAL(0, Wire1.begins);
    TEST_ASSERT_EQUAL(INPUT, pinModes[16]); TEST_ASSERT_EQUAL(INPUT, pinModes[5]);
    TEST_ASSERT_EQUAL(-1, pinModes[17]);
    for (const auto& tx : Wire.transmissions) { TEST_ASSERT_EQUAL(0x50, tx.address); TEST_ASSERT_EQUAL(2, tx.bytes.size()); }
    TEST_ASSERT_NOT_NULL(strstr(f.pages.content.c_str(), "NotProvisioned"));
    TEST_ASSERT_NOT_NULL(strstr(f.pages.content.c_str(), "Mini 0.8"));
    TEST_ASSERT_NOT_NULL(strstr(f.pages.content.c_str(), "Weather 0.2"));
    TEST_ASSERT_NULL(strstr(f.pages.content.c_str(), "descriptorModuleSlot"));
    TEST_ASSERT_NULL(strstr(f.pages.content.c_str(), "WriteAndVerifyThisBoardIdentityTheActive"));
}
void test_confirmation_and_invalid_identity_cannot_touch_i2c() {
    Fixture f; f.form(false);
    f.server.request("/hardware/board-identity/write", HTTP_POST);
    TEST_ASSERT_EQUAL(400, f.pages.status); TEST_ASSERT_EQUAL(0, Wire.begins);
    f.form(); f.server.arguments["serialNumber"] = "bad<value>";
    f.server.request("/device/board-identity/write", HTTP_POST);
    TEST_ASSERT_EQUAL(400, f.pages.status); TEST_ASSERT_EQUAL(0, Wire.begins);
    auto identity = f.identity(); identity.revision.minor = 9;
    TEST_ASSERT_EQUAL(BoardProvisioningStatus::InvalidIdentity, f.administration.provisionBoardIdentity(identity, true).status);
    TEST_ASSERT_EQUAL(0, Wire.begins);
}
void test_external_write_is_verified_and_preserves_rest_of_eeprom_without_restart() {
    Fixture f;
    auto& data = Wire.storage(0x50); std::fill(data.begin(), data.end(), 0xa5);
    f.form(); f.server.request("/hardware/board-identity/write", HTTP_POST);
    TEST_ASSERT_EQUAL(200, f.pages.status);
    TEST_ASSERT_EQUAL(1, Wire.begins); TEST_ASSERT_EQUAL(1, Wire.ends);
    TEST_ASSERT_NULL(f.bus.wire(I2CBus::I2C0));
    TEST_ASSERT_NULL(strstr(f.pages.content.c_str(), "action='/restart'"));
    TEST_ASSERT_NOT_NULL(strstr(f.pages.content.c_str(), "No tester restart required"));
    BoardIdentity decoded{};
    TEST_ASSERT_EQUAL(BoardIdentityStatus::Valid, BoardIdentityCodec::decode(data.data(), 32, decoded));
    TEST_ASSERT_EQUAL_STRING("MINI-EXTERNAL", decoded.serialNumber.value);
    for (size_t i = 32; i < data.size(); ++i) TEST_ASSERT_EQUAL_HEX8(0xa5, data[i]);
    TEST_ASSERT_EQUAL_STRING("MINI-EXTERNAL", f.administration.provisioningBoardIdentity().serialNumber.value);
    const auto result = f.administration.provisionBoardIdentity(f.identity(), true);
    TEST_ASSERT_EQUAL(BoardProvisioningStatus::Success, result.status);
    TEST_ASSERT_FALSE(result.rebootRequired);
}
void test_missing_eeprom_and_readback_failure_are_not_successes() {
    Fixture f; Wire.defaultResult = 2;
    f.form(); f.server.request("/hardware/board-identity/write", HTTP_POST);
    TEST_ASSERT_EQUAL(400, f.pages.status);
    TEST_ASSERT_EQUAL(1, Wire.ends); TEST_ASSERT_NULL(f.bus.wire(I2CBus::I2C0));
    Wire.defaultResult = 0; Wire.readLimit = 0;
    const auto result = f.administration.provisionBoardIdentity(f.identity(), true);
    TEST_ASSERT_EQUAL(BoardProvisioningStatus::ReadbackFailed, result.status);
    TEST_ASSERT_FALSE(result.rebootRequired);
    TEST_ASSERT_EQUAL(2, Wire.ends); TEST_ASSERT_NULL(f.bus.wire(I2CBus::I2C0));
    TEST_ASSERT_EQUAL_STRING("", f.administration.provisioningBoardIdentity().serialNumber.value);
}
void test_bus_initialization_failure_also_releases_connection() {
    Fixture f; Wire.beginOk = false;
    const auto read = f.administration.refresh();
    TEST_ASSERT_EQUAL(BoardIdentityStatus::StorageUnavailable, read.status);
    TEST_ASSERT_EQUAL(1, Wire.ends); TEST_ASSERT_EQUAL(0, Wire.transmissions.size());
    const auto result = f.administration.provisionBoardIdentity(f.identity(), true);
    TEST_ASSERT_EQUAL(BoardProvisioningStatus::WriteFailed, result.status);
    TEST_ASSERT_EQUAL(2, Wire.ends); TEST_ASSERT_NULL(f.bus.wire(I2CBus::I2C0));
}
void test_running_connector_test_blocks_reads_and_writes_including_alias() {
    Fixture f;
    TEST_ASSERT_TRUE(f.tests.load(Testing::weatherBoard()));
    f.simulation.connect(*f.tests.board(), 0, Testing::SimulatedFault::None);
    TEST_ASSERT_TRUE(f.tests.start(0));
    f.server.request("/hardware", HTTP_GET); TEST_ASSERT_EQUAL(409, f.pages.status);
    f.form(); f.server.request("/hardware/board-identity/write", HTTP_POST); TEST_ASSERT_EQUAL(409, f.pages.status);
    f.server.request("/device/board-identity/write", HTTP_POST); TEST_ASSERT_EQUAL(409, f.pages.status);
    TEST_ASSERT_EQUAL(BoardIdentityStatus::StorageUnavailable, f.administration.refresh().status);
    TEST_ASSERT_EQUAL(BoardProvisioningStatus::WriteFailed, f.administration.provisionBoardIdentity(f.identity(), true).status);
    TEST_ASSERT_EQUAL(0, Wire.begins);
    f.tests.abort();
    f.server.request("/hardware/board-identity/write", HTTP_POST); TEST_ASSERT_EQUAL(200, f.pages.status);
    TEST_ASSERT_EQUAL(1, Wire.begins);
}
void test_hotplug_read_failure_clears_previous_dut_and_status_is_localized() {
    Fixture f;
    TEST_ASSERT_EQUAL(BoardProvisioningStatus::Success, f.administration.provisionBoardIdentity(f.identity(), true).status);
    f.pages.locale = Locale::GermanGermany;
    f.server.request("/hardware", HTTP_GET);
    TEST_ASSERT_NOT_NULL(strstr(f.pages.content.c_str(), "Gelesene Identität"));
    TEST_ASSERT_NOT_NULL(strstr(f.pages.content.c_str(), "MINI-EXTERNAL"));
    Wire.defaultResult = 2;
    f.server.request("/hardware", HTTP_GET);
    TEST_ASSERT_NOT_NULL(strstr(f.pages.content.c_str(), "StorageUnavailable"));
    TEST_ASSERT_NULL(strstr(f.pages.content.c_str(), "MINI-EXTERNAL"));
    TEST_ASSERT_EQUAL_STRING("", f.administration.provisioningBoardIdentity().serialNumber.value);
}
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_no_bus_at_boot_and_no_module_routes);
    RUN_TEST(test_overview_reads_only_external_address_then_releases_bus);
    RUN_TEST(test_confirmation_and_invalid_identity_cannot_touch_i2c);
    RUN_TEST(test_external_write_is_verified_and_preserves_rest_of_eeprom_without_restart);
    RUN_TEST(test_missing_eeprom_and_readback_failure_are_not_successes);
    RUN_TEST(test_bus_initialization_failure_also_releases_connection);
    RUN_TEST(test_running_connector_test_blocks_reads_and_writes_including_alias);
    RUN_TEST(test_hotplug_read_failure_clears_previous_dut_and_status_is_localized);
    return UNITY_END();
}
