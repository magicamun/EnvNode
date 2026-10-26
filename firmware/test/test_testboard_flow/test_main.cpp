// EnvNode: see repository LICENSE.
#include <unity.h>
#include <cstring>
#include "TestBoardSettings.h"
#include "TestBoardRestart.h"
#include "TestBoardWeb.h"
#include "RecentLogStore.h"
#include "Update.h"
#include "testing/TestWebRoutes.h"
#include <vector>
using namespace EnvNode::Testing;
using namespace EnvNode;
UpdateClass Update;
HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t n) { return n; }
struct Logger : ILogger {
    void begin(unsigned long) override {}
    void println(const char*) override {}
    void printf(const char*, ...) override {}
};
struct Wifi : IWiFiService {
    void begin() override {} void loop() override {}
    bool connected() const override { return false; }
    bool inSetupAccessPointMode() const override { return true; }
    String ipAddress() const override { return "192.168.4.1"; }
    String hostname() const override { return "EnvNode-Testboard"; }
    int32_t rssi() const override { return 0; }
};
struct Time : ITimeService {
    void begin() override {} void loop() override {}
    bool synchronized() const override { return false; }
    time_t now() const override { return 0; }
    bool localCivilTime(tm&) const override { return false; }
    String iso8601Utc() const override { return {}; }
    String iso8601Local() const override { return {}; }
    String iso8601Local(time_t) const override { return {}; }
    uint32_t epoch() const override { return 0; }
};
struct RestartDriver : IDeviceRestartDriver {
    int calls = 0;
    void restart() override { ++calls; }
};
struct Fixture {
    Logger logger; TestBoardSettings settings; Wifi wifi; Time time;
    LocaleFormatter formatter{settings.base()}; RecentLogStore logs;
    RestartDriver driver; TestBoardRestart restart{logger, &driver}; OTAService ota{logger, restart};
    WebHost host{logger, wifi};
    TestBoardWeb web{host, settings, wifi, time, formatter, ota, restart, logs,
        {"0.2.2", "0.2.2+test", "1", "abc", "testboard", "now", "clean", false}};
    SimulatedFixture fixture;
    TestRunner runner{fixture};
    TestWebRoutes routes{host, web, runner, fixture};
    Fixture() { settings.load(); web.attachTestRoutes(routes); web.begin(); }
    WebServer& server() { return host.server(); }
};
void setUp() {
    Preferences::failBeginNamespace() = String(); Preferences::failClearNamespace() = String();
    Preferences::failStringWrites() = false; Preferences::failUIntKey() = String();
    Update = UpdateClass();
    arduinoTestMillis() = 0;
    for (const char* name : {"testboard", "testboard0", "testboard1", "test_meta", "weather", "weather_meta"}) {
        Preferences p; p.begin(name); p.clear();
    }
}
void tearDown() {}
void finish(TestRunner& runner, uint32_t& now) {
    for (size_t i = 0; runner.phase() == Phase::Running && i < 600; ++i) {
        now += 20; runner.tick(now);
    }
    TEST_ASSERT_EQUAL(Phase::Review, runner.phase());
}
struct IndependentProbe : ITestFixture {
    std::vector<std::pair<unsigned, unsigned>> paths;
    bool selected = false, busy = false, selectOk = true;
    uint8_t contact = 0, socket = 0; unsigned releases = 0;
    bool select(Pigtail type, uint8_t c, uint8_t s) override {
        TEST_ASSERT_EQUAL(Pigtail::Gpio, type);
        contact = c; socket = s; paths.push_back({c,s}); selected = selectOk; return selectOk;
    }
    Sample read(bool& connected) override {
        if (busy) return Sample::Busy;
        connected = contact == 1 ? socket == 14 || socket == 32 || socket == 38
            : contact == 2 ? socket == 19 : contact == 3 && socket == 27;
        return Sample::Ready;
    }
    void release() override { selected = false; ++releases; }
};
void test_weather_profile_matches_direct_nets_and_marks_sys_rail_uncovered() {
    const auto& board = weatherBoard();
    TEST_ASSERT_EQUAL(19, board.count);
    const auto& gpio = board.connectors[0];
    TEST_ASSERT_EQUAL_STRING("GPIO16", gpio.id);
    TEST_ASSERT_EQUAL_UINT64((1ULL << 13) | (1ULL << 31) | (1ULL << 37), gpio.contacts[0].expected);
    TEST_ASSERT_EQUAL_UINT64(1ULL << 26, gpio.contacts[2].expected);
    TEST_ASSERT_EQUAL_UINT64(0, gpio.contacts[3].expected);
    TEST_ASSERT_EQUAL_STRING("+3V3_DEVKIT", board.socket[0].net);
    TEST_ASSERT_EQUAL_STRING("+3V3_SYS", gpio.contacts[3].net);
}
void test_all_152_pairs_use_logical_pin_numbers_and_wait_for_operator() {
    IndependentProbe probe; TestRunner runner(probe); uint32_t now = 0;
    TEST_ASSERT_FALSE(runner.start(now)); TEST_ASSERT_FALSE(runner.next());
    TEST_ASSERT_TRUE(runner.load(weatherBoard())); TEST_ASSERT_TRUE(runner.start(now));
    TEST_ASSERT_FALSE(runner.start(now)); TEST_ASSERT_FALSE(runner.next());
    finish(runner, now);
    TEST_ASSERT_EQUAL(152, runner.completed()); TEST_ASSERT_EQUAL(152, probe.paths.size());
    for (size_t i = 0; i < 152; ++i) {
        TEST_ASSERT_EQUAL(i / 38 + 1, probe.paths[i].first);
        TEST_ASSERT_EQUAL(i % 38 + 1, probe.paths[i].second);
    }
    TEST_ASSERT_FALSE(probe.selected);
    TEST_ASSERT_EQUAL(Result::Connected, runner.result(13));
    TEST_ASSERT_EQUAL(Result::Connected, runner.result(31));
    TEST_ASSERT_EQUAL(Result::Connected, runner.result(37));
    TEST_ASSERT_EQUAL(Result::Isolated, runner.result(0));
    TEST_ASSERT_EQUAL(Verdict::Incomplete, runner.verdict(0));
    TEST_ASSERT_EQUAL(Verdict::Passed, runner.contactVerdict(0, 0));
    TEST_ASSERT_EQUAL(Verdict::Incomplete, runner.contactVerdict(0, 3));
    runner.tick(now + 10000); TEST_ASSERT_EQUAL(0, runner.step());
    TEST_ASSERT_TRUE(runner.next()); TEST_ASSERT_EQUAL(Phase::AwaitingConnection, runner.phase());
}
void test_open_and_unexpected_connections_are_failures() {
    for (auto fault : {SimulatedFault::OpenContact, SimulatedFault::ExtraConnection}) {
        SimulatedFixture fixture; TestRunner runner(fixture); uint32_t now = 0;
        runner.load(weatherBoard()); fixture.connect(weatherBoard(), 0, fault); runner.start(now); finish(runner, now);
        TEST_ASSERT_EQUAL(Verdict::Failed, runner.verdict(0));
        TEST_ASSERT_EQUAL(Verdict::Failed, runner.contactVerdict(0, 0));
        TEST_ASSERT_EQUAL(fault == SimulatedFault::OpenContact ? Result::Open : Result::Connected, runner.result(13));
        if (fault == SimulatedFault::ExtraConnection) TEST_ASSERT_EQUAL(Result::Unexpected, runner.result(0));
    }
}
void test_complete_plan_includes_module_532_pairs_without_false_full_pass() {
    SimulatedFixture fixture; TestRunner runner(fixture); uint32_t now = 0; size_t total = 0;
    runner.load(weatherBoard());
    for (size_t step = 0; step < weatherBoard().count; ++step) {
        TEST_ASSERT_EQUAL(Phase::AwaitingConnection, runner.phase());
        fixture.connect(weatherBoard(), step, SimulatedFault::None); runner.start(now); finish(runner, now);
        total += runner.completed();
        if (step == 18) TEST_ASSERT_EQUAL(532, runner.completed());
        TEST_ASSERT_EQUAL(Verdict::Incomplete, runner.verdict(step)); runner.next();
    }
    TEST_ASSERT_EQUAL(3268, total); TEST_ASSERT_EQUAL(Phase::Complete, runner.phase());
    TEST_ASSERT_EQUAL(Verdict::Incomplete, runner.overall()); TEST_ASSERT_FALSE(runner.next());
}
void test_abort_releases_fixture_and_reload_clears_previous_results() {
    IndependentProbe fixture; TestRunner runner(fixture); runner.load(weatherBoard()); runner.start(0); runner.tick(20);
    runner.abort(); TEST_ASSERT_FALSE(fixture.selected); TEST_ASSERT_EQUAL(Phase::Aborted, runner.phase());
    const auto paths = fixture.paths.size(); runner.tick(5000); TEST_ASSERT_EQUAL(paths, fixture.paths.size());
    TEST_ASSERT_FALSE(runner.start(5000)); runner.load(weatherBoard());
    TEST_ASSERT_EQUAL(0, runner.completed()); TEST_ASSERT_EQUAL(Verdict::Untested, runner.verdict(0));
}
void test_busy_backend_times_out_and_select_error_never_reads_a_stale_path() {
    IndependentProbe fixture; fixture.busy = true; TestRunner runner(fixture); runner.load(weatherBoard()); runner.start(0);
    runner.tick(20); TEST_ASSERT_EQUAL(0, runner.completed());
    runner.tick(2000); TEST_ASSERT_EQUAL(1, runner.completed()); TEST_ASSERT_EQUAL(Result::Error, runner.result(0));
    runner.abort(); fixture.busy = false; fixture.selectOk = false; runner.load(weatherBoard()); runner.start(0);
    runner.tick(20); TEST_ASSERT_EQUAL(Result::Error, runner.result(0));
}
void test_stale_and_invalid_web_actions_do_not_advance_the_session() {
    Fixture f;
    f.server().arguments = {{"action", "load"}, {"generation", "0"}, {"board", weatherBoard().id}};
    TEST_ASSERT_TRUE(f.server().request("/test/action", HTTP_POST)); TEST_ASSERT_EQUAL(200, f.server().responseStatus);
    f.server().arguments = {{"action", "start"}, {"generation", "0"}, {"fault", "none"}};
    f.server().request("/test/action", HTTP_POST); TEST_ASSERT_EQUAL(409, f.server().responseStatus);
    TEST_ASSERT_EQUAL(Phase::AwaitingConnection, f.runner.phase());
    f.server().request("/test/state", HTTP_GET);
    TEST_ASSERT_EQUAL('}', f.server().responseBody.back());
    TEST_ASSERT_NOT_NULL(strstr(f.server().responseBody.c_str(), "\"verdicts\":[0,0"));
    TEST_ASSERT_NOT_NULL(strstr(f.server().responseBody.c_str(), "\"contacts\":\"000"));
    f.server().arguments["generation"] = String(f.runner.generation()); f.server().arguments["fault"] = "invalid";
    f.server().request("/test/action", HTTP_POST); TEST_ASSERT_EQUAL(400, f.server().responseStatus);
    f.server().arguments["fault"] = "none";
    f.server().request("/test/action", HTTP_POST); TEST_ASSERT_EQUAL(200, f.server().responseStatus);
    TEST_ASSERT_EQUAL(Phase::Running, f.runner.phase());
    f.server().request("/test/action", HTTP_POST); TEST_ASSERT_EQUAL(409, f.server().responseStatus);
    TEST_ASSERT_EQUAL(0, f.runner.completed());
}
void test_actual_test_page_and_definition_apis_are_simulation_only() {
    Fixture f; f.server().request("/test", HTTP_GET);
    TEST_ASSERT_EQUAL(200, f.server().responseStatus);
    TEST_ASSERT_NOT_NULL(strstr(f.server().responseBody.c_str(), "SIMULATION"));
    TEST_ASSERT_NOT_NULL(strstr(f.server().responseBody.c_str(), "/test/ui.js"));
    TEST_ASSERT_NOT_NULL(strstr(f.server().responseBody.c_str(), "test-matrix"));
    f.server().request("/test/definition", HTTP_GET);
    TEST_ASSERT_NOT_NULL(strstr(f.server().responseBody.c_str(), "weather-0.2-pcb0.2.1"));
    TEST_ASSERT_NOT_NULL(strstr(f.server().responseBody.c_str(), "\"covered\":false"));
    f.server().request("/test/state", HTTP_GET);
    TEST_ASSERT_NOT_NULL(strstr(f.server().responseBody.c_str(), "\"simulation\":true"));
    TEST_ASSERT_NOT_NULL(strstr(f.server().responseBody.c_str(), "\"phase\":0"));
}
void test_free_selection_preserves_matrices_and_retest_replaces_only_selected_result() {
    SimulatedFixture fixture; TestRunner runner(fixture); uint32_t now = 0;
    TEST_ASSERT_FALSE(runner.selectConnector(0));
    runner.load(weatherBoard()); fixture.connect(weatherBoard(), 0, SimulatedFault::OpenContact);
    runner.start(now); finish(runner, now);
    TEST_ASSERT_EQUAL(Verdict::Failed, runner.verdict(0));
    TEST_ASSERT_EQUAL(Result::Open, runner.result(13));
    TEST_ASSERT_TRUE(runner.selectConnector(1));
    fixture.connect(weatherBoard(), 1, SimulatedFault::None); runner.start(now); finish(runner, now);
    TEST_ASSERT_EQUAL(Verdict::Incomplete, runner.verdict(1));
    TEST_ASSERT_TRUE(runner.selectConnector(0));
    TEST_ASSERT_EQUAL(Phase::Review, runner.phase());
    TEST_ASSERT_EQUAL(152, runner.completed());
    TEST_ASSERT_EQUAL(Result::Open, runner.result(13));
    fixture.connect(weatherBoard(), 0, SimulatedFault::None);
    TEST_ASSERT_TRUE(runner.start(now));
    TEST_ASSERT_EQUAL(Verdict::Untested, runner.verdict(0));
    TEST_ASSERT_EQUAL(Result::Pending, runner.result(13));
    TEST_ASSERT_EQUAL(Verdict::Incomplete, runner.verdict(1));
    finish(runner, now);
    TEST_ASSERT_EQUAL(Verdict::Incomplete, runner.verdict(0));
    TEST_ASSERT_EQUAL(Result::Connected, runner.result(13));
    runner.selectConnector(1);
    TEST_ASSERT_EQUAL(152, runner.completed());
    TEST_ASSERT_EQUAL(Result::Connected, runner.result(13));
    TEST_ASSERT_EQUAL(Verdict::Passed, runner.contactVerdict(1, 0));
}
void test_out_of_order_completion_next_untested_and_selection_after_completion() {
    SimulatedFixture fixture; TestRunner runner(fixture); uint32_t now = 0;
    runner.load(weatherBoard()); runner.selectConnector(18);
    fixture.connect(weatherBoard(), 18, SimulatedFault::None); runner.start(now); finish(runner, now);
    TEST_ASSERT_TRUE(runner.next()); TEST_ASSERT_EQUAL(0, runner.step());
    TEST_ASSERT_EQUAL(Phase::AwaitingConnection, runner.phase());
    for (size_t i = 18; i-- > 0;) {
        TEST_ASSERT_TRUE(runner.selectConnector(i));
        fixture.connect(weatherBoard(), i, SimulatedFault::None); runner.start(now); finish(runner, now);
    }
    TEST_ASSERT_EQUAL(0, runner.step());
    TEST_ASSERT_TRUE(runner.next()); TEST_ASSERT_EQUAL(Phase::Complete, runner.phase());
    TEST_ASSERT_TRUE(runner.selectConnector(18));
    TEST_ASSERT_EQUAL(Phase::Review, runner.phase()); TEST_ASSERT_EQUAL(532, runner.completed());
    const auto generation = runner.generation();
    TEST_ASSERT_FALSE(runner.selectConnector(weatherBoard().count));
    TEST_ASSERT_EQUAL(generation, runner.generation());
    runner.abort(); TEST_ASSERT_TRUE(runner.selectConnector(0));
    TEST_ASSERT_EQUAL(Result::Connected, runner.result(13));
}
void test_web_selection_and_retest_are_strict_and_locked_while_running() {
    Fixture f;
    f.runner.load(weatherBoard());
    auto select = [&](const char* index, uint32_t generation) {
        f.server().arguments = {{"action", "select"}, {"connector", index}, {"generation", String(generation)}};
        f.server().request("/test/action", HTTP_POST);
    };
    select("18", f.runner.generation());
    TEST_ASSERT_EQUAL(200, f.server().responseStatus); TEST_ASSERT_EQUAL(18, f.runner.step());
    const auto generation = f.runner.generation();
    for (const char* index : {"-1", "19", "xyz", "42949672960"}) {
        select(index, generation); TEST_ASSERT_EQUAL(409, f.server().responseStatus);
        TEST_ASSERT_EQUAL(18, f.runner.step());
    }
    select("0", generation - 1); TEST_ASSERT_EQUAL(409, f.server().responseStatus);
    f.fixture.connect(weatherBoard(), 18, SimulatedFault::None); f.runner.start(0);
    uint32_t now = 0; finish(f.runner, now);
    f.server().arguments = {{"action", "start"}, {"generation", String(f.runner.generation())}, {"fault", "none"}};
    f.server().request("/test/action", HTTP_POST);
    TEST_ASSERT_EQUAL(200, f.server().responseStatus);
    TEST_ASSERT_EQUAL(Verdict::Untested, f.runner.verdict(18));
    select("0", f.runner.generation()); TEST_ASSERT_EQUAL(409, f.server().responseStatus);
    TEST_ASSERT_EQUAL(18, f.runner.step()); TEST_ASSERT_EQUAL(Phase::Running, f.runner.phase());
}
void test_mini_profile_has_independent_slot_wiring_and_explicit_address_ties() {
    const auto& board = miniBoard();
    TEST_ASSERT_EQUAL(2, boardCount()); TEST_ASSERT_EQUAL(8, board.count);
    TEST_ASSERT_EQUAL_PTR(&board, findBoard("mini-0.8-pcb0.8.1"));
    TEST_ASSERT_NULL(findBoard("unknown")); TEST_ASSERT_NULL(boardAt(2));
    TEST_ASSERT_EQUAL_UINT64(1ULL << 4, board.connectors[0].contacts[2].expected);
    TEST_ASSERT_EQUAL_UINT64(1ULL << 5, board.connectors[1].contacts[2].expected);
    const auto& a = board.connectors[6]; const auto& b = board.connectors[7];
    TEST_ASSERT_EQUAL_STRING("J1", a.id); TEST_ASSERT_EQUAL_STRING("J2", b.id);
    TEST_ASSERT_EQUAL_UINT64((1ULL << 13) | (1ULL << 31) | (1ULL << 37), a.contacts[2].expected);
    TEST_ASSERT_EQUAL_UINT64(0, b.contacts[2].expected);
    TEST_ASSERT_EQUAL_STRING("EEPROM_A0_B", b.contacts[2].net);
    TEST_ASSERT_EQUAL_UINT64(1ULL << 7, a.contacts[8].expected);
    TEST_ASSERT_EQUAL_UINT64(1ULL << 6, b.contacts[8].expected);
    TEST_ASSERT_EQUAL_UINT64(1ULL << 28, a.contacts[13].expected);
    TEST_ASSERT_EQUAL_UINT64(1ULL << 10, b.contacts[13].expected);
}
void test_mini_plan_has_1976_comparisons_and_nine_uncovered_contacts() {
    SimulatedFixture fixture; TestRunner runner(fixture); uint32_t now = 0; size_t total = 0, uncovered = 0;
    runner.load(miniBoard());
    for (size_t i = 0; i < miniBoard().count; ++i) {
        fixture.connect(miniBoard(), i, SimulatedFault::None); runner.start(now); finish(runner, now);
        total += runner.completed();
        for (size_t p = 0; p < miniBoard().connectors[i].count; ++p)
            uncovered += miniBoard().connectors[i].contacts[p].expected == 0;
        runner.next();
    }
    TEST_ASSERT_EQUAL(1976, total); TEST_ASSERT_EQUAL(9, uncovered);
    TEST_ASSERT_EQUAL(Phase::Complete, runner.phase()); TEST_ASSERT_EQUAL(Verdict::Incomplete, runner.overall());
}
void test_catalog_profile_assets_and_board_switch_never_mix_results() {
    Fixture f; f.server().request("/test/catalog", HTTP_GET);
    TEST_ASSERT_NOT_NULL(strstr(f.server().responseBody.c_str(), weatherBoard().id));
    TEST_ASSERT_NOT_NULL(strstr(f.server().responseBody.c_str(), miniBoard().id));
    f.runner.load(weatherBoard()); f.fixture.connect(weatherBoard(), 0, SimulatedFault::None);
    f.runner.start(0); uint32_t now = 0; finish(f.runner, now);
    f.server().arguments = {{"action", "load"}, {"board", miniBoard().id}, {"generation", String(f.runner.generation())}};
    f.server().request("/test/action", HTTP_POST);
    TEST_ASSERT_EQUAL(200, f.server().responseStatus); TEST_ASSERT_EQUAL_PTR(&miniBoard(), f.runner.board());
    TEST_ASSERT_NOT_NULL(strstr(f.server().responseBody.c_str(), "\"board\":\"mini-0.8-pcb0.8.1\""));
    TEST_ASSERT_EQUAL(0, f.runner.completed()); TEST_ASSERT_EQUAL(Verdict::Untested, f.runner.verdict(0));
    f.server().arguments.clear(); f.server().request("/test/definition", HTTP_GET);
    TEST_ASSERT_NOT_NULL(strstr(f.server().responseBody.c_str(), "Slot B"));
    f.server().arguments["board"] = "unknown";
    f.server().request("/test/definition", HTTP_GET); TEST_ASSERT_EQUAL(404, f.server().responseStatus);
    f.server().arguments["board"] = miniBoard().id;
    f.server().request("/test/board.svg", HTTP_GET); TEST_ASSERT_EQUAL(200, f.server().responseStatus);
    TEST_ASSERT_EQUAL_HEX8(0x1f, static_cast<uint8_t>(f.server().responseBody[0]));
    TEST_ASSERT_EQUAL_HEX8(0x8b, static_cast<uint8_t>(f.server().responseBody[1]));
}
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_weather_profile_matches_direct_nets_and_marks_sys_rail_uncovered);
    RUN_TEST(test_all_152_pairs_use_logical_pin_numbers_and_wait_for_operator);
    RUN_TEST(test_open_and_unexpected_connections_are_failures);
    RUN_TEST(test_complete_plan_includes_module_532_pairs_without_false_full_pass);
    RUN_TEST(test_abort_releases_fixture_and_reload_clears_previous_results);
    RUN_TEST(test_busy_backend_times_out_and_select_error_never_reads_a_stale_path);
    RUN_TEST(test_stale_and_invalid_web_actions_do_not_advance_the_session);
    RUN_TEST(test_actual_test_page_and_definition_apis_are_simulation_only);
    RUN_TEST(test_free_selection_preserves_matrices_and_retest_replaces_only_selected_result);
    RUN_TEST(test_out_of_order_completion_next_untested_and_selection_after_completion);
    RUN_TEST(test_web_selection_and_retest_are_strict_and_locked_while_running);
    RUN_TEST(test_mini_profile_has_independent_slot_wiring_and_explicit_address_ties);
    RUN_TEST(test_mini_plan_has_1976_comparisons_and_nine_uncovered_contacts);
    RUN_TEST(test_catalog_profile_assets_and_board_switch_never_mix_results);
    return UNITY_END();
}
