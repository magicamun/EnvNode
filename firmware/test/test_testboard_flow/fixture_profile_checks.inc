// EnvNode: see repository LICENSE.
void test_fixture_profiles_are_independent_and_resolve_all_40_inputs() {
    FixtureSelection selection;
    TEST_ASSERT_EQUAL_STRING(fixtureRevision01().id,selection.definition().id);
    for(unsigned code=0;code<4;++code) {
        TEST_ASSERT_TRUE(selection.selectCode(code));TEST_ASSERT_EQUAL(code,selection.code());
        bool used[3][16]={};
        for(unsigned i=1;i<=40;++i){MuxAddress source{},dest{};
            TEST_ASSERT_TRUE(resolveFixturePath(selection.definition(),Pigtail::Module,2,i,source,dest));
            TEST_ASSERT_EQUAL(1,source.device);TEST_ASSERT_EQUAL(6,source.channel);
            TEST_ASSERT_FALSE(used[dest.device][dest.channel]);used[dest.device][dest.channel]=true;
        }
        TEST_ASSERT_EQUAL(code>=2,selection.controller().directTestDrive);
        const auto r=selection.definition().routing;
        TEST_ASSERT_EQUAL(code%2?0:13,r.socket[38].channel);
        TEST_ASSERT_EQUAL(code%2?15:14,r.socket[39].channel);
        TEST_ASSERT_EQUAL(code%2?1:2,r.socket[14].device);
        TEST_ASSERT_EQUAL(code%2?1:7,r.socket[14].channel);
    }
    TEST_ASSERT_FALSE(selection.select("unknown","input-0.1"));TEST_ASSERT_EQUAL(3,selection.code());
    TEST_ASSERT_FALSE(selection.selectCode(4));TEST_ASSERT_EQUAL(3,selection.code());
}
void test_fixture_selection_changes_physical_routing_without_boot_gpio_effects() {
    struct Io : IDiagnosticIo {unsigned switches=0;DiagnosticPath path{{0,0},-1,0,false};
        void selectDiagnostic(const DiagnosticPath& p) override{++switches;path=p;}
        uint16_t sampleDiagnostic()override{return 0;}void releaseDiagnostic()override{}}
    io;FixtureSelection selection;PhysicalFixture fixture(io,[](){return 0;},&selection);
    TEST_ASSERT_EQUAL(0,io.switches);selection.selectCode(3);
    TEST_ASSERT_TRUE(fixture.select(Pigtail::Gpio,1,15));
    TEST_ASSERT_EQUAL(1,io.path.inputDevice);TEST_ASSERT_EQUAL(1,io.path.inputChannel);
    fixture.release();selection.selectCode(0);fixture.select(Pigtail::Gpio,1,15);
    TEST_ASSERT_EQUAL(2,io.path.inputDevice);TEST_ASSERT_EQUAL(7,io.path.inputChannel);fixture.release();
}
void test_fixture_web_change_requires_confirmation_clears_results_and_handles_persistence_failure() {
    Fixture f;FixtureSelection selection;unsigned changes=0;bool persist=true;
    f.routes.attachSelection(selection,[&](){++changes;},[&](unsigned code){TEST_ASSERT_EQUAL(3,code);return persist;});
    uint32_t now=0;f.runner.load(weatherBoard());runDirected(f.runner,f.fixture,0,now);
    f.server().arguments={{"action","fixture"},{"generation",String(f.runner.generation())},{"controller","controller-0.2"},{"adapter","esp32-devkitc-0.3"}};
    f.request("/test/action",HTTP_POST);TEST_ASSERT_EQUAL(400,f.server().responseStatus);TEST_ASSERT_EQUAL(0,selection.code());
    f.server().arguments["confirm"]="yes";persist=false;f.request("/test/action",HTTP_POST);
    TEST_ASSERT_EQUAL(503,f.server().responseStatus);TEST_ASSERT_EQUAL(0,selection.code());TEST_ASSERT_EQUAL(152,f.runner.completed());
    persist=true;f.request("/test/action",HTTP_POST);TEST_ASSERT_EQUAL(200,f.server().responseStatus);
    TEST_ASSERT_EQUAL(3,selection.code());TEST_ASSERT_EQUAL(1,changes);TEST_ASSERT_EQUAL(0,f.runner.completed());
    TEST_ASSERT_EQUAL(Verdict::Untested,f.runner.verdict(0));
    TEST_ASSERT_NOT_NULL(strstr(f.server().responseBody.c_str(),"\"adapterProfile\":\"esp32-devkitc-0.3\""));
    runDirected(f.runner,f.fixture,0,now);
    f.server().arguments["generation"]=String(f.runner.generation());
    f.request("/test/action",HTTP_POST);TEST_ASSERT_EQUAL(200,f.server().responseStatus);
    TEST_ASSERT_EQUAL(152,f.runner.completed());TEST_ASSERT_EQUAL(1,changes); // Reapplying the same profiles preserves results.
    f.server().arguments["generation"]=String(f.runner.generation());f.routes.setHardwareBusy([](){return true;});
    f.request("/test/action",HTTP_POST);TEST_ASSERT_EQUAL(409,f.server().responseStatus);TEST_ASSERT_EQUAL(1,changes);
    f.routes.setHardwareBusy([](){return false;});f.runner.start(now);
    f.server().arguments["generation"]=String(f.runner.generation());f.request("/test/action",HTTP_POST);
    TEST_ASSERT_EQUAL(409,f.server().responseStatus);TEST_ASSERT_EQUAL(1,changes);
}
void test_fixture_profile_settings_survive_reload_and_factory_reset() {
    TestBoardSettings first;TEST_ASSERT_TRUE(first.load());TEST_ASSERT_EQUAL(0,first.fixtureProfile());
    TEST_ASSERT_TRUE(first.saveFixtureProfile(3));TestBoardSettings second;TEST_ASSERT_TRUE(second.load());TEST_ASSERT_EQUAL(3,second.fixtureProfile());
    Preferences::failUIntKey()="fixtureProfile";TEST_ASSERT_FALSE(second.saveFixtureProfile(1));TEST_ASSERT_EQUAL(3,second.fixtureProfile());
    Preferences::failUIntKey()=String();TEST_ASSERT_FALSE(second.saveFixtureProfile(4));
    TEST_ASSERT_TRUE(second.reset());TestBoardSettings reset;TEST_ASSERT_TRUE(reset.load());TEST_ASSERT_EQUAL(0,reset.fixtureProfile());
}
