// EnvNode: see repository LICENSE.
#include <unity.h>
#include "testing/GuidedSelfTest.h"
#include "Update.h"
#include <vector>
using namespace EnvNode::Testing;
UpdateClass Update;
HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*,size_t n) { return n; }
void pinMode(unsigned char,int) {}
void digitalWrite(unsigned char,int) {}
void setUp() {}
void tearDown() {}
struct Io : IDiagnosticIo {
    DiagnosticPath current{{0,0},-1,0,false};
    size_t test=0; int fault=0; bool enabled=false; unsigned releases=0,reads=0;
    std::vector<DiagnosticPath> selections;
    void selectDiagnostic(const DiagnosticPath& p) override { current=p;enabled=true;selections.push_back(p); }
    void releaseDiagnostic() override { enabled=false;++releases; }
    uint16_t sampleDiagnostic() override {
        ++reads;
        if(fault==2)return 2400; // foreign voltage / short to high
        if(!enabled||!current.high||fault==1)return 0;
        const auto target=GuidedSelfTest::output(test<GuidedSelfTest::OutputCount?test:9);
        bool connected=current.output.device==target.device&&current.output.channel==target.channel;
        if(test<GuidedSelfTest::OutputCount)connected &= current.inputDevice==-1;
        else {
            const auto in=fixtureRevision01().routing.socket[test-GuidedSelfTest::OutputCount];
            connected &= current.inputDevice==in.device&&current.inputChannel==in.channel;
        }
        if(fault==5&&(current.output.device==255||current.inputDevice<0))connected=true;
        if(fault==3)connected=true; // unwanted cross-channel conduction
        return connected ? (fault==4?4095:2273) : 0;
    }
};
void finish(GuidedSelfTest& runner,uint32_t start=0) {
    for(uint32_t n=2;runner.running()&&n<=12000;n+=2)runner.tick(start+n);
    TEST_ASSERT_FALSE(runner.running());
}
void test_all_contacts_use_the_committed_routing_and_release_outputs() {
    for(size_t i=0;i<GuidedSelfTest::Count;++i) {
        Io io;io.test=i;GuidedSelfTest runner(io);
        TEST_ASSERT_TRUE(runner.start(i,0));finish(runner);
        TEST_ASSERT_EQUAL(unsigned(SelfTestVerdict::Passed),unsigned(runner.result(i).verdict));
        TEST_ASSERT_EQUAL(0,runner.result(i).low);TEST_ASSERT_EQUAL(2273,runner.result(i).high);
        TEST_ASSERT_EQUAL(0,runner.result(i).isolation);TEST_ASSERT_FALSE(io.enabled);
        TEST_ASSERT_EQUAL(8*(i<22?24:42),io.reads);
    }
}
void test_open_short_crosstalk_and_saturation_fail() {
    for(int fault=1;fault<=5;++fault)for(size_t i : {size_t(9),size_t(22)}) {
        Io io;io.test=i;io.fault=fault;GuidedSelfTest runner(io);runner.start(i,0);finish(runner);
        TEST_ASSERT_EQUAL(unsigned(SelfTestVerdict::Failed),unsigned(runner.result(i).verdict));
        TEST_ASSERT_FALSE(io.enabled);
    }
}
void test_invalid_and_busy_start_do_not_switch_hardware() {
    Io io;GuidedSelfTest runner(io);TEST_ASSERT_FALSE(runner.start(GuidedSelfTest::Count,0));
    TEST_ASSERT_TRUE(io.selections.empty());TEST_ASSERT_TRUE(runner.start(0,0));
    const size_t before=io.selections.size();TEST_ASSERT_FALSE(runner.start(1,0));
    TEST_ASSERT_EQUAL(before,io.selections.size());runner.cancel();TEST_ASSERT_FALSE(io.enabled);
    TEST_ASSERT_EQUAL(unsigned(SelfTestVerdict::Aborted),unsigned(runner.result(0).verdict));
}
void test_settling_timeout_and_clock_rollover() {
    Io io;GuidedSelfTest runner(io);runner.start(0,0);runner.tick(14);TEST_ASSERT_EQUAL(0,io.reads);
    runner.tick(15);TEST_ASSERT_EQUAL(1,io.reads);runner.tick(15);TEST_ASSERT_EQUAL(1,io.reads);
    runner.tick(10001);TEST_ASSERT_FALSE(runner.running());TEST_ASSERT_FALSE(io.enabled);
    TEST_ASSERT_EQUAL(unsigned(SelfTestVerdict::Aborted),unsigned(runner.result(0).verdict));
    TEST_ASSERT_TRUE(runner.start(0,0xffffff00));finish(runner,0xffffff00);
    TEST_ASSERT_EQUAL(unsigned(SelfTestVerdict::Passed),unsigned(runner.result(0).verdict));
}
void test_results_remain_independent_and_clear_is_inactive_only() {
    Io io;GuidedSelfTest runner(io);runner.start(0,0);finish(runner);
    io.test=1;runner.start(1,2000);runner.clear();TEST_ASSERT_TRUE(runner.running());
    TEST_ASSERT_EQUAL(unsigned(SelfTestVerdict::Passed),unsigned(runner.result(0).verdict));
    runner.cancel();runner.clear();
    for(size_t i=0;i<GuidedSelfTest::Count;++i)TEST_ASSERT_EQUAL(unsigned(SelfTestVerdict::Untested),unsigned(runner.result(i).verdict));
}
int main() { UNITY_BEGIN();
    RUN_TEST(test_all_contacts_use_the_committed_routing_and_release_outputs);
    RUN_TEST(test_open_short_crosstalk_and_saturation_fail);
    RUN_TEST(test_invalid_and_busy_start_do_not_switch_hardware);
    RUN_TEST(test_settling_timeout_and_clock_rollover);
    RUN_TEST(test_results_remain_independent_and_clear_is_inactive_only);
    return UNITY_END(); }
