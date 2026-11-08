// EnvNode: see repository LICENSE.
struct ReportProbe : SimulatedFixture {
    bool faulty=true;
    Sample read(bool& connected) override {return faulty?Sample::Error:SimulatedFixture::read(connected);}
    bool trace(MeasurementTrace& value) const override {
        value=MeasurementTrace();value.reason=MeasurementReason::HighAmbiguous;
        value.lowCount=value.highCount=8;
        for(unsigned i=0;i<8;++i){value.low[i]=0;value.high[i]=i%2?2200:400;}
        return true;
    }
};
void test_fault_history_retains_raw_samples_across_successful_retest_and_is_bounded() {
    ReportProbe fixture;TestRunner runner(fixture);uint32_t now=0;runner.load(weatherBoard());
    runDirected(runner,fixture,0,now);
    TEST_ASSERT_EQUAL(128,runner.diagnosticCount());TEST_ASSERT_EQUAL(24,runner.diagnosticsDropped());
    TEST_ASSERT_EQUAL(24,runner.diagnosticAt(0).cell);TEST_ASSERT_EQUAL(151,runner.diagnosticAt(127).cell);
    const auto attempt=runner.diagnosticAt(0).attempt;
    TEST_ASSERT_EQUAL(8,runner.diagnosticAt(0).trace.highCount);
    TEST_ASSERT_EQUAL(400,runner.diagnosticAt(0).trace.high[0]);
    TEST_ASSERT_EQUAL(2200,runner.diagnosticAt(0).trace.high[1]);
    fixture.faulty=false;runDirected(runner,fixture,0,now);
    TEST_ASSERT_EQUAL(128,runner.diagnosticCount());TEST_ASSERT_EQUAL(attempt,runner.diagnosticAt(0).attempt);
    TEST_ASSERT_NOT_EQUAL(Verdict::Failed,runner.verdict(0));
    runner.load(weatherBoard());TEST_ASSERT_EQUAL(0,runner.diagnosticCount());TEST_ASSERT_EQUAL(0,runner.diagnosticsDropped());
}
void test_report_pages_are_read_only_guarded_and_include_unselected_connectors() {
    Fixture f;uint32_t now=0;f.runner.load(weatherBoard());
    runDirected(f.runner,f.fixture,0,now,SimulatedFault::OpenContact);
    runDirected(f.runner,f.fixture,10,now);const auto generation=f.runner.generation();const auto step=f.runner.step();
    f.server().arguments={{"generation",String(generation)},{"connector","0"},{"contact","0"}};
    TEST_ASSERT_TRUE(f.request("/test/report",HTTP_GET));TEST_ASSERT_EQUAL(200,f.server().responseStatus);
    TEST_ASSERT_NOT_NULL(strstr(f.server().responseBody.c_str(),"\"results\":"));
    TEST_ASSERT_NOT_NULL(strstr(f.server().responseBody.c_str(),"\"observedResult\":4"));
    TEST_ASSERT_NOT_NULL(strstr(f.server().responseBody.c_str(),"\"reason\":1,\"low\":[],\"high\":[]")); // Simulation never invents ADC readings.
    TEST_ASSERT_EQUAL(step,f.runner.step());TEST_ASSERT_EQUAL(generation,f.runner.generation());TEST_ASSERT_EQUAL(0,f.real.selects);
    f.server().arguments.erase("contact");f.request("/test/report",HTTP_GET);
    TEST_ASSERT_EQUAL(200,f.server().responseStatus);
    const auto whole=f.server().responseBody;
    TEST_ASSERT_NOT_NULL(strstr(whole.c_str(),"\"contact\":0"));
    const auto from=whole.find("\"results\":\"")+11;
    TEST_ASSERT_EQUAL(152,whole.find('"',from)-from);
    TEST_ASSERT_NOT_NULL(strstr(whole.c_str(),"\"contactVerdicts\":\"2113\""));
    f.server().arguments["generation"]="0";f.request("/test/report",HTTP_GET);TEST_ASSERT_EQUAL(409,f.server().responseStatus);
    f.server().arguments["generation"]=String(generation);f.server().arguments["contact"]="14";
    f.request("/test/report",HTTP_GET);TEST_ASSERT_EQUAL(400,f.server().responseStatus);
    f.server().arguments["contact"]="0";f.runner.start(now);
    f.server().arguments["generation"]=String(f.runner.generation());f.request("/test/report",HTTP_GET);
    TEST_ASSERT_EQUAL(409,f.server().responseStatus);
}
void test_report_history_pagination_is_bounded_and_keeps_previous_attempts() {
    Fixture f;ReportProbe probe;TestRunner runner(probe);uint32_t now=0;
    f.server().routes.clear();
    TestWebRoutes routes(f.host,f.web,runner,f.fixture);routes.registerRoutes(f.server());
    runner.load(weatherBoard());runDirected(runner,probe,0,now);
    f.server().arguments={{"generation",String(runner.generation())},{"connector","0"},{"contact","3"}};
    f.request("/test/report",HTTP_GET);TEST_ASSERT_EQUAL(200,f.server().responseStatus);
    const auto first=f.server().responseBody;
    TEST_ASSERT_NOT_NULL(strstr(first.c_str(),"\"nextOffset\":106"));
    TEST_ASSERT_NOT_NULL(strstr(first.c_str(),"\"high\":[400,2200,400,2200,400,2200,400,2200]"));
    TEST_ASSERT_TRUE(first.length()<6500);
    f.server().arguments["offset"]="106";f.request("/test/report",HTTP_GET);
    TEST_ASSERT_NOT_NULL(strstr(f.server().responseBody.c_str(),"\"nextOffset\":122"));
    f.server().arguments["offset"]="122";f.request("/test/report",HTTP_GET);
    TEST_ASSERT_NOT_NULL(strstr(f.server().responseBody.c_str(),"\"nextOffset\":null"));
}
