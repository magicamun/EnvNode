// EnvNode: see repository LICENSE.
// Independent fault injection: observations are not derived from runner results.
struct DirectedProbe : SimulatedFixture {
    unsigned contact=0,input=0;
    bool missingSda=false,missingGround=false,otherShort=false,ambiguous=false;
    bool select(Pigtail type,uint8_t c,uint8_t i) override {
        contact=c;input=i;return SimulatedFixture::select(type,c,i);
    }
    Sample read(bool& connected) override {
        auto status=SimulatedFixture::read(connected);
        if(missingSda&&contact==3)connected=false;
        if(missingGround&&contact==1&&(input==14||input==32||input==38))connected=false;
        if(otherShort&&contact==1&&input==9)connected=true;
        if(ambiguous&&contact==1&&input==33)return Sample::Error;
        return status;
    }
};
static void runDirected(TestRunner& runner,SimulatedFixture& fixture,size_t connector,uint32_t& now,
                        SimulatedFault fault=SimulatedFault::None) {
    TEST_ASSERT_TRUE(runner.selectConnector(connector));
    fixture.connect(*runner.board(),connector,fault);TEST_ASSERT_TRUE(runner.start(now));finish(runner,now);
}
void test_directed_paths_wait_for_valid_countercheck_and_recompute_history() {
    SimulatedFixture fixture;TestRunner runner(fixture);uint32_t now=0;
    TEST_ASSERT_TRUE(runner.load(weatherBoard()));TEST_ASSERT_NOT_NULL(runner.policy());
    runDirected(runner,fixture,0,now);
    TEST_ASSERT_EQUAL(Result::DirectedPending,runner.result(32));
    TEST_ASSERT_EQUAL(Result::DirectedPending,runner.result(35));
    TEST_ASSERT_EQUAL(Verdict::Incomplete,runner.contactVerdict(0,0));
    runDirected(runner,fixture,10,now);
    TEST_ASSERT_EQUAL(1,runner.reverseEvidence(0));TEST_ASSERT_EQUAL(1,runner.reverseEvidence(1));
    TEST_ASSERT_EQUAL(Result::DirectedBlocked,runner.result(2*38+13));
    runner.selectConnector(0);
    TEST_ASSERT_EQUAL(Result::DirectedConnected,runner.result(32));
    TEST_ASSERT_EQUAL(Result::DirectedConnected,runner.result(35));
    TEST_ASSERT_EQUAL(Verdict::Passed,runner.contactVerdict(0,0));
    TEST_ASSERT_EQUAL(Verdict::Incomplete,runner.verdict(0)); // Legacy SYS rail has no positive path.
}
void test_directed_short_overrides_good_counterchecks_and_retest_removes_proof() {
    SimulatedFixture fixture;TestRunner runner(fixture);uint32_t now=0;runner.load(weatherBoard());
    runDirected(runner,fixture,0,now);runDirected(runner,fixture,10,now);
    runDirected(runner,fixture,11,now,SimulatedFault::DirectedShort);
    TEST_ASSERT_EQUAL(2,runner.reverseEvidence(0));TEST_ASSERT_EQUAL(2,runner.reverseEvidence(1));
    runner.selectConnector(0);TEST_ASSERT_EQUAL(Result::Unexpected,runner.result(32));
    TEST_ASSERT_EQUAL(Verdict::Failed,runner.verdict(0));
    runDirected(runner,fixture,10,now); // Good connector cannot override a short elsewhere.
    TEST_ASSERT_EQUAL(2,runner.reverseEvidence(0));
    runDirected(runner,fixture,11,now);runner.selectConnector(0);
    TEST_ASSERT_EQUAL(Result::DirectedConnected,runner.result(32));
    // Remove the other good proof, then restarting the last proof must invalidate old green results immediately.
    runDirected(runner,fixture,10,now,SimulatedFault::OpenContact);
    // OpenContact affects GND only; SDA/SCL still prove their own reverse paths.
    runner.selectConnector(10);fixture.connect(weatherBoard(),10,SimulatedFault::None);
    runner.start(now);runner.abort();
    runner.selectConnector(11);fixture.connect(weatherBoard(),11,SimulatedFault::None);runner.start(now);
    TEST_ASSERT_EQUAL(0,runner.reverseEvidence(0));runner.abort();runner.selectConnector(0);
    TEST_ASSERT_EQUAL(Result::DirectedPending,runner.result(32));
    TEST_ASSERT_EQUAL(Verdict::Incomplete,runner.contactVerdict(0,0));
}
void test_missing_positive_paths_never_certify_reverse_isolation() {
    DirectedProbe fixture;TestRunner runner(fixture);uint32_t now=0;runner.load(weatherBoard());
    runDirected(runner,fixture,0,now);fixture.missingSda=true;
    runDirected(runner,fixture,10,now);
    TEST_ASSERT_EQUAL(0,runner.reverseEvidence(0));TEST_ASSERT_EQUAL(1,runner.reverseEvidence(1));
    TEST_ASSERT_EQUAL(Result::Open,runner.result(2*38+32));
    TEST_ASSERT_EQUAL(Verdict::Failed,runner.verdict(10));
    runner.selectConnector(0);TEST_ASSERT_EQUAL(Result::DirectedPending,runner.result(32));
    TEST_ASSERT_EQUAL(Result::DirectedConnected,runner.result(35));
    fixture.missingSda=false;runDirected(runner,fixture,10,now);
    fixture.missingGround=true;runDirected(runner,fixture,0,now);
    TEST_ASSERT_EQUAL(Result::DirectedPending,runner.result(32));
    TEST_ASSERT_EQUAL(Verdict::Failed,runner.contactVerdict(0,0));
}
void test_aborted_countercheck_does_not_leave_stale_evidence() {
    SimulatedFixture fixture;TestRunner runner(fixture);uint32_t now=0;runner.load(weatherBoard());
    runDirected(runner,fixture,0,now);runDirected(runner,fixture,10,now);
    runner.selectConnector(10);fixture.connect(weatherBoard(),10,SimulatedFault::None);runner.start(now);
    // SDA row has been observed, but connector has not finished.
    for(unsigned i=0;i<3*38;++i){now+=20;runner.tick(now);}
    TEST_ASSERT_EQUAL(0,runner.reverseEvidence(0));runner.abort();runner.selectConnector(0);
    TEST_ASSERT_EQUAL(Result::DirectedPending,runner.result(32));
    TEST_ASSERT_TRUE(runner.resetForModeChange());TEST_ASSERT_EQUAL(0,runner.reverseEvidence(0));
    TEST_ASSERT_EQUAL(Result::Pending,runner.result(32));
}
void test_policy_is_bound_to_exact_known_electrical_revision() {
    BoardDefinition board=weatherBoard();TEST_ASSERT_NOT_NULL(measurementPolicy(board));
    board.id="unknown-board";TEST_ASSERT_NULL(measurementPolicy(board));
    SimulatedFixture fixture;TestRunner runner(fixture);uint32_t now=0;runner.load(board);
    // Feed known-board electrical observations to an unreviewed profile: no exemption.
    fixture.connect(weatherBoard(),0,SimulatedFault::None);runner.start(now);finish(runner,now);
    TEST_ASSERT_EQUAL(Result::Unexpected,runner.result(32));
    board=weatherBoard();board.pcbRevision="0.2.2";TEST_ASSERT_NULL(measurementPolicy(board));
    board=weatherBoard();SocketContact socket[38];std::copy_n(board.socket,38,socket);
    socket[32].net="OTHER";board.socket=socket;TEST_ASSERT_NULL(measurementPolicy(board));
    board=weatherBoard();Connector connectors[MaxConnectors];std::copy_n(board.connectors,board.count,connectors);
    Contact contacts[4];std::copy_n(connectors[0].contacts,4,contacts);contacts[0].expected^=1;
    connectors[0].contacts=contacts;board.connectors=connectors;TEST_ASSERT_NULL(measurementPolicy(board));
}
void test_directed_rules_do_not_hide_other_shorts_or_measurement_errors() {
    DirectedProbe fixture;TestRunner runner(fixture);uint32_t now=0;runner.load(weatherBoard());
    runDirected(runner,fixture,10,now);fixture.otherShort=true;fixture.ambiguous=true;
    runDirected(runner,fixture,0,now);
    TEST_ASSERT_EQUAL(Result::Unexpected,runner.result(8));
    TEST_ASSERT_EQUAL(Result::Error,runner.result(32));TEST_ASSERT_EQUAL(Verdict::Failed,runner.verdict(0));
}
void test_direction_policy_and_evidence_are_exposed_by_web_api() {
    Fixture f;uint32_t now=0;f.runner.load(weatherBoard());
    f.request("/test/state",HTTP_GET);
    TEST_ASSERT_NOT_NULL(strstr(f.server().responseBody.c_str(),"\"measurementPolicy\":\"eeprom-ground-clamp-v1\""));
    TEST_ASSERT_NOT_NULL(strstr(f.server().responseBody.c_str(),"\"to\":\"I2C0_SDA\",\"reverse\":0"));
    runDirected(f.runner,f.fixture,10,now);f.request("/test/state",HTTP_GET);
    TEST_ASSERT_NOT_NULL(strstr(f.server().responseBody.c_str(),"\"to\":\"I2C0_SDA\",\"reverse\":1"));
    runDirected(f.runner,f.fixture,10,now,SimulatedFault::DirectedShort);f.request("/test/state",HTTP_GET);
    TEST_ASSERT_NOT_NULL(strstr(f.server().responseBody.c_str(),"\"to\":\"I2C0_SDA\",\"reverse\":2"));
    TEST_ASSERT_TRUE(f.runner.load(*findBoard("weather-0.3-pcb0.3.0")));
    runDirected(f.runner,f.fixture,0,now);f.request("/test/state",HTTP_GET);
    TEST_ASSERT_NOT_NULL(strstr(f.server().responseBody.c_str(),"\"measurementPolicy\":\"weather-ground-clamps-v2\""));
    TEST_ASSERT_NOT_NULL(strstr(f.server().responseBody.c_str(),"\"to\":\"Net-(D2-K)\",\"reverse\":1,\"reverseSource\":\"+3V3_SYS\""));
}

void test_weather_rail_path_uses_pigtail_drive_and_requires_reference_anchor() {
    struct RailProbe : SimulatedFixture {
        uint8_t contact=0,input=0;bool openReference=false,railShort=false;
        bool select(Pigtail p,uint8_t c,uint8_t i) override {contact=c;input=i;return SimulatedFixture::select(p,c,i);}
        Sample read(bool& connected) override {
            auto status=SimulatedFixture::read(connected);
            if(contact==4&&input==39&&openReference)connected=false;
            if(contact==4&&(input==14||input==32||input==38)&&railShort)connected=true;
            return status;
        }
    } fixture;
    TestRunner runner(fixture);uint32_t now=0;const auto& board=*findBoard("weather-0.3-pcb0.3.0");
    TEST_ASSERT_TRUE(runner.load(board));TEST_ASSERT_EQUAL(3,runner.policy()->count);
    runDirected(runner,fixture,0,now);
    TEST_ASSERT_EQUAL(1,runner.reverseEvidence(2));
    TEST_ASSERT_EQUAL(Result::DirectedConnected,runner.result(38));
    TEST_ASSERT_EQUAL(Result::DirectedBlocked,runner.result(3*40+13));
    TEST_ASSERT_EQUAL(Result::DirectedPending,runner.result(32)); // I2C proof still missing.
    fixture.openReference=true;runDirected(runner,fixture,0,now);
    TEST_ASSERT_EQUAL(0,runner.reverseEvidence(2));TEST_ASSERT_EQUAL(Result::DirectedPending,runner.result(38));
    TEST_ASSERT_EQUAL(Result::Open,runner.result(3*40+38));TEST_ASSERT_EQUAL(Verdict::Failed,runner.verdict(0));
    fixture.openReference=false;fixture.railShort=true;runDirected(runner,fixture,0,now);
    TEST_ASSERT_EQUAL(2,runner.reverseEvidence(2));TEST_ASSERT_EQUAL(Result::Unexpected,runner.result(38));
    TEST_ASSERT_EQUAL(Result::Unexpected,runner.result(3*40+13));
    fixture.railShort=false;runDirected(runner,fixture,0,now);runDirected(runner,fixture,10,now);
    runner.selectConnector(0);TEST_ASSERT_EQUAL(Verdict::Passed,runner.verdict(0));
    TEST_ASSERT_EQUAL(Result::DirectedConnected,runner.result(38));
    // An unmeasured board revision must not inherit the new rail exception.
    TEST_ASSERT_TRUE(runner.load(*findBoard("mini-0.9-pcb0.9.0")));
    TEST_ASSERT_EQUAL(2,runner.policy()->count);
}
