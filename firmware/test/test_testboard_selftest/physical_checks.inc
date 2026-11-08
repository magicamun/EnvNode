// EnvNode: see repository LICENSE.
#include "testing/PhysicalFixture.h"
#include "testing/TestRunner.h"
struct MeasurementIo : IDiagnosticIo {
    DiagnosticPath p{{0,0},-1,0,false};int low=0,high=2273,reads=0,releases=0;bool active=false;
    void selectDiagnostic(const DiagnosticPath& path) override {p=path;active=true;}
    void releaseDiagnostic() override {active=false;++releases;}
    uint16_t sampleDiagnostic() override {++reads;return p.high?high:low;}
};
void test_physical_measurement_classifies_only_observed_levels() {
    for(int high : {0,150,400,799,800,1500,2273,3800,4095}) {
        uint32_t now=0;MeasurementIo io;io.high=high;PhysicalFixture fixture(io,[&](){return now;});
        TEST_ASSERT_TRUE(fixture.select(Pigtail::Module,2,1));
        TEST_ASSERT_EQUAL(1,io.p.output.device);TEST_ASSERT_EQUAL(6,io.p.output.channel);
        TEST_ASSERT_EQUAL(0,io.p.inputDevice);TEST_ASSERT_EQUAL(7,io.p.inputChannel);
        bool connected=false;auto sample=ITestFixture::Sample::Busy;
        for(now=0;now<500&&sample==ITestFixture::Sample::Busy;now+=2)sample=fixture.read(connected);
        const bool valid=high<=200||(high>=800&&high<=3800);
        TEST_ASSERT_EQUAL(unsigned(valid?ITestFixture::Sample::Ready:ITestFixture::Sample::Error),unsigned(sample));
        TEST_ASSERT_EQUAL(high>=800&&high<=3800,connected);
        TEST_ASSERT_EQUAL(high,fixture.readings().high);TEST_ASSERT_FALSE(io.active);
        MeasurementTrace trace;TEST_ASSERT_TRUE(fixture.trace(trace));
        TEST_ASSERT_EQUAL(8,trace.lowCount);TEST_ASSERT_EQUAL(8,trace.highCount);
        TEST_ASSERT_EQUAL(high,trace.high[7]);
        TEST_ASSERT_EQUAL(unsigned(valid?MeasurementReason::None:MeasurementReason::HighAmbiguous),unsigned(trace.reason));
    }
}
void test_physical_low_fault_timeout_invalid_and_release() {
    uint32_t now=0;MeasurementIo io;PhysicalFixture fixture(io,[&](){return now;});
    TEST_ASSERT_FALSE(fixture.select(Pigtail::Gpio,5,1));TEST_ASSERT_EQUAL(0,io.reads);
    TEST_ASSERT_TRUE(fixture.select(Pigtail::I2c,1,40));
    TEST_ASSERT_EQUAL(0,io.p.output.device);TEST_ASSERT_EQUAL(7,io.p.output.channel);
    TEST_ASSERT_EQUAL(2,io.p.inputDevice);TEST_ASSERT_EQUAL(14,io.p.inputChannel);
    bool connected=false;fixture.read(connected);TEST_ASSERT_EQUAL(0,io.reads);
    io.low=300;auto sample=ITestFixture::Sample::Busy;
    for(now=0;now<500&&sample==ITestFixture::Sample::Busy;now+=2)sample=fixture.read(connected);
    TEST_ASSERT_EQUAL(unsigned(ITestFixture::Sample::Error),unsigned(sample));TEST_ASSERT_FALSE(io.active);
    MeasurementTrace trace;fixture.trace(trace);TEST_ASSERT_EQUAL(8,trace.lowCount);TEST_ASSERT_EQUAL(0,trace.highCount);
    TEST_ASSERT_EQUAL(300,trace.low[0]);TEST_ASSERT_EQUAL(unsigned(MeasurementReason::LowNotLow),unsigned(trace.reason));
    io.low=0;now=1000;fixture.select(Pigtail::Gpio,1,1);now=1500;
    TEST_ASSERT_EQUAL(unsigned(ITestFixture::Sample::Error),unsigned(fixture.read(connected)));TEST_ASSERT_FALSE(io.active);
    fixture.trace(trace);TEST_ASSERT_EQUAL(unsigned(MeasurementReason::Timeout),unsigned(trace.reason));
    TEST_ASSERT_EQUAL(0,trace.lowCount);TEST_ASSERT_EQUAL(0,trace.highCount);
    fixture.select(Pigtail::Gpio,1,1);fixture.release();TEST_ASSERT_FALSE(io.active);
    TEST_ASSERT_EQUAL(unsigned(ITestFixture::Sample::Error),unsigned(fixture.read(connected)));
}
void test_selectable_fixture_does_not_drive_at_boot_or_keep_old_outputs() {
    uint32_t now=0;MeasurementIo io;PhysicalFixture real(io,[&](){return now;});
    class Sim : public ITestFixture {public:unsigned releases=0;bool select(Pigtail,uint8_t,uint8_t)override{return true;}
        Sample read(bool& c)override{c=true;return Sample::Ready;}void release()override{++releases;}} sim;
    SelectableFixture choice(sim,&real,false);TEST_ASSERT_EQUAL(0,io.releases);TEST_ASSERT_FALSE(choice.simulated());
    choice.select(Pigtail::Gpio,1,1);TEST_ASSERT_TRUE(io.active);
    choice.setSimulation(true);TEST_ASSERT_FALSE(io.active);TEST_ASSERT_TRUE(choice.simulated());
    TEST_ASSERT_EQUAL(1,io.releases);choice.setSimulation(false);TEST_ASSERT_EQUAL(1,sim.releases);
    SelectableFixture unavailable(sim);TEST_ASSERT_FALSE(unavailable.setSimulation(false));
}
void test_real_driver_runs_a_complete_40_input_matrix_without_using_expectations() {
    struct BoardIo : IDiagnosticIo {
        DiagnosticPath path{{0,0},-1,0,false};bool active=false,broken=false;
        void selectDiagnostic(const DiagnosticPath& p)override{path=p;active=true;}
        void releaseDiagnostic()override{active=false;}
        uint16_t sampleDiagnostic()override{
            if(!active||!path.high||path.output.device!=0)return 0;
            // Independently specified synthetic wiring, including two diode-level paths.
            const int devices[]={2,2,0,0},channels[]={14,13,7,6};
            const int contact=int(path.output.channel)-8;
            if(contact<0||contact>3||(broken&&contact==0))return 0;
            return path.inputDevice==devices[contact]&&path.inputChannel==channels[contact]?(contact<2?1500:2273):0;
        }
    } io;
    SocketContact socket[MaxInputs];for(size_t i=0;i<MaxInputs;++i)socket[i]={uint8_t(i+1),"synthetic",0,0};
    Contact contacts[]={{1,"gnd",0,0,1ULL<<39},{2,"rail",0,0,1ULL<<38},{3,"a",0,0,1ULL},{4,"b",0,0,2ULL}};
    Connector connector{"J",Pigtail::Gpio,4,contacts};
    BoardDefinition board{"synthetic","Synthetic","0.1","0.1",socket,&connector,1,40};
    uint32_t now=0;PhysicalFixture fixture(io,[&](){return now;});TestRunner runner(fixture);
    TEST_ASSERT_TRUE(runner.load(board));TEST_ASSERT_TRUE(runner.start(now));
    for(;now<90000&&runner.phase()==Phase::Running;now+=2)runner.tick(now);
    TEST_ASSERT_EQUAL(160,runner.completed());TEST_ASSERT_EQUAL(Verdict::Passed,runner.verdict(0));TEST_ASSERT_FALSE(io.active);
    io.broken=true;TEST_ASSERT_TRUE(runner.start(now));
    for(uint32_t end=now+90000;now<end&&runner.phase()==Phase::Running;now+=2)runner.tick(now);
    TEST_ASSERT_EQUAL(Verdict::Failed,runner.verdict(0));TEST_ASSERT_EQUAL(Result::Open,runner.result(39));
    TEST_ASSERT_FALSE(io.active);
}
void test_real_driver_rejects_boundary_crossing_samples_and_survives_clock_wrap() {
    struct Noisy : MeasurementIo {
        uint16_t sampleDiagnostic()override{++reads;return p.high?(reads%2?400:2200):0;}
    } io;
    uint32_t now=0xffffff00;PhysicalFixture fixture(io,[&](){return now;});
    TEST_ASSERT_TRUE(fixture.select(Pigtail::Gpio,1,1));bool connected=false;auto sample=ITestFixture::Sample::Busy;
    for(unsigned i=0;i<250&&sample==ITestFixture::Sample::Busy;++i){now+=2;sample=fixture.read(connected);}
    TEST_ASSERT_EQUAL(unsigned(ITestFixture::Sample::Error),unsigned(sample));TEST_ASSERT_FALSE(connected);TEST_ASSERT_FALSE(io.active);
}
