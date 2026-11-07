// EnvNode: see repository LICENSE.
struct IsolationIo : Io {
    int kind=0;unsigned sample=0;
    void selectDiagnostic(const DiagnosticPath& p)override{Io::selectDiagnostic(p);sample=0;}
    uint16_t sampleDiagnostic()override{
        ++sample;
        if(current.high&&test>=22&&current.inputDevice==0&&current.inputChannel==0)
            return kind==1?226:sample==3?223:0;
        if(kind==2&&current.high&&test>=22&&current.inputDevice<0)return 240;
        if(kind==3&&current.high&&test<22&&current.output.device==0&&current.output.channel==8)
            return sample==3?254:0;
        if(kind==4&&current.high&&test>=22&&current.inputDevice==0&&current.inputChannel==2)return 223;
        return Io::sampleDiagnostic();
    }
};
void test_isolation_single_spike_records_channel_samples_and_unchanged_failure() {
    IsolationIo io;io.test=22+6;GuidedSelfTest runner(io);runner.start(io.test,0);finish(runner);
    const auto& r=runner.result(io.test);
    TEST_ASSERT_EQUAL(unsigned(SelfTestVerdict::Failed),unsigned(r.verdict));
    TEST_ASSERT_EQUAL(223,r.isolation);TEST_ASSERT_EQUAL(8,r.isolationPath); // J4/8 -> U4/I0
    TEST_ASSERT_EQUAL(27,r.isolationMean);TEST_ASSERT_EQUAL(0,r.isolationMinimum);TEST_ASSERT_EQUAL(1,r.isolationAbove);
    TEST_ASSERT_EQUAL(1,r.isolationTotalAbove);TEST_ASSERT_EQUAL(320,r.isolationTotalSamples);TEST_ASSERT_EQUAL(1,r.isolationBadWindows);
    for(size_t i=0;i<8;++i)TEST_ASSERT_EQUAL(i==2?223:0,r.isolationSamples[i]);
}
void test_isolation_steady_voltage_and_disabled_path_are_distinguishable() {
    for(int kind : {1,2}){
        IsolationIo io;io.test=22+6;io.kind=kind;GuidedSelfTest runner(io);runner.start(io.test,0);finish(runner);
        const auto& r=runner.result(io.test);
        TEST_ASSERT_EQUAL(kind==1?8:0,r.isolationPath);
        TEST_ASSERT_EQUAL(kind==1?226:240,r.isolationMean);TEST_ASSERT_EQUAL(kind==1?226:240,r.isolationMinimum);
        TEST_ASSERT_EQUAL(8,r.isolationAbove);
        TEST_ASSERT_EQUAL(kind==1?8:9,r.isolationTotalAbove);
        TEST_ASSERT_EQUAL(kind==1?1:2,r.isolationBadWindows);
    }
}
void test_output_isolation_and_equal_peaks_keep_the_correct_contact() {
    IsolationIo io;io.test=9;io.kind=3;GuidedSelfTest runner(io);runner.start(io.test,0);finish(runner);
    const auto& r=runner.result(io.test);TEST_ASSERT_EQUAL(1,r.isolationPath); // Output J1/1 -> U1/I8
    TEST_ASSERT_EQUAL(254,r.isolation);TEST_ASSERT_EQUAL(31,r.isolationMean);TEST_ASSERT_EQUAL(176,r.isolationTotalSamples);
    IsolationIo tie;tie.test=22+6;tie.kind=4;GuidedSelfTest second(tie);second.start(tie.test,0);finish(second);
    const auto& v=second.result(tie.test);TEST_ASSERT_EQUAL(6,v.isolationPath); // persistent 223 beats isolated 223 at J4/8
    TEST_ASSERT_EQUAL(223,v.isolationMean);TEST_ASSERT_EQUAL(8,v.isolationAbove);TEST_ASSERT_EQUAL(9,v.isolationTotalAbove);
}
void test_isolation_details_reset_on_retest_clear_and_remain_per_contact() {
    IsolationIo io;io.test=28;GuidedSelfTest runner(io);runner.start(28,0);finish(runner);
    io.test=29;runner.start(29,2000);finish(runner,2000);
    TEST_ASSERT_EQUAL(8,runner.result(28).isolationPath);
    runner.start(28,4000);TEST_ASSERT_EQUAL(255,runner.result(28).isolationPath);TEST_ASSERT_EQUAL(0,runner.result(28).isolationTotalSamples);
    runner.cancel();runner.clear();
    for(size_t i=0;i<GuidedSelfTest::Count;++i){TEST_ASSERT_EQUAL(255,runner.result(i).isolationPath);TEST_ASSERT_EQUAL(0,runner.result(i).isolationTotalAbove);}
}

void test_selectable_settling_waits_without_changing_sampling_or_decisions() {
    for(uint16_t settle : {15,50,100}) {
        IsolationIo io;io.test=28;GuidedSelfTest runner(io);
        TEST_ASSERT_FALSE(runner.start(io.test,0,16));TEST_ASSERT_TRUE(io.selections.empty());
        TEST_ASSERT_TRUE(runner.start(io.test,0,settle));
        runner.tick(settle-1);TEST_ASSERT_EQUAL(0,io.reads);
        runner.tick(settle);TEST_ASSERT_EQUAL(1,io.reads);
        finish(runner,settle);
        const auto& r=runner.result(io.test);
        TEST_ASSERT_EQUAL(settle,r.settleMs);TEST_ASSERT_EQUAL(320,r.isolationTotalSamples);
        TEST_ASSERT_EQUAL(223,r.isolation);TEST_ASSERT_EQUAL(1,r.isolationTotalAbove);
        TEST_ASSERT_EQUAL(unsigned(SelfTestVerdict::Failed),unsigned(r.verdict));
    }
}
