// EnvNode-owned: see ../../../LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
#include <unity.h>
#include <Arduino.h>
#include <RainDetectorPolicy.h>
#include "RainFrequencyConversion.h"
#include "RainFrequencyAcquisition.h"
using namespace EnvNode;
HardwareSerial Serial;
// Native firmware build links the existing logger/GPIO adapters as well.
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t count) { return count; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}
void setUp() {}
void tearDown() {}
void test_shared_policy_wetting_end_and_dry_are_independent() {
    RainDetectorPolicy policy;
    policy.enable();
    for (uint64_t t=250; t<=6250; t+=250) policy.update(t,34000,0);
    TEST_ASSERT_TRUE(policy.update(6500,30000,0).wetting);
    for (uint64_t t=6750;t<=72000;t+=250) policy.update(t,30000,100);
    TEST_ASSERT_TRUE(policy.result().rain == RainDetectorRainState::EndEstimated);
    TEST_ASSERT_TRUE(policy.result().heating);
    for (uint64_t t=72250;t<=83000;t+=250) policy.update(t,33000,100);
    TEST_ASSERT_TRUE(policy.result().dry);
    TEST_ASSERT_FALSE(policy.result().heating);
}
void test_signal_loss_aborts_cycle_but_preserves_arming() {
    RainDetectorPolicy policy;
    policy.enable();
    for (uint64_t t=250;t<=6250;t+=250) policy.update(t,34000,0);
    TEST_ASSERT_TRUE(policy.update(6500,30000,0).heating);
    TEST_ASSERT_FALSE(policy.update(6750,NAN,100).heating);
    TEST_ASSERT_TRUE(policy.enabled());
    TEST_ASSERT_TRUE(policy.result().rain == RainDetectorRainState::Unknown);
    for (uint64_t t=7000;t<=14000;t+=250) policy.update(t,30000,0);
    TEST_ASSERT_FALSE(policy.result().heating);
    TEST_ASSERT_TRUE(policy.update(14250,29000,0).heating);
}
void test_config_is_atomic_and_bounded() {
    RainDetectorPolicy policy;
    RainDetectorPolicyConfig config;
    config.endSeconds=29;
    TEST_ASSERT_FALSE(policy.configure(config));
    TEST_ASSERT_EQUAL_UINT(60,policy.config().endSeconds);
    config.endSeconds=300;
    TEST_ASSERT_TRUE(policy.configure(config));
    policy.enable();
    TEST_ASSERT_FALSE(policy.configure(config));
    policy.disable();
    TEST_ASSERT_FALSE(policy.result().heating);
}
class TestFrequencyInput : public IRainFrequencyInput {
public:
    bool startOk = true;
    unsigned stopped = 0;
    RainFrequencyPoll next = RainFrequencyPoll::Pending;
    RainFrequencyWindow window;
    bool begin(uint8_t gpio) override { return startOk && gpio == 27; }
    RainFrequencyPoll poll(RainFrequencyWindow& value) override { value = window; return next; }
    void stop() override { ++stopped; }
};
class TestFrequencyObserver : public IRainFrequencyObserver {
public:
    unsigned count = 0;
    RainFrequencyWindow last;
    void onRainFrequency(const RainFrequencyWindow& window) override { ++count; last = window; }
};
void test_frequency_conversion_detects_zero_wrap_and_gate_errors() {
    auto good = rainFrequencyWindow(5000000000000ULL, 5000000250000ULL, 10000, false);
    TEST_ASSERT_TRUE(good.valid());
    TEST_ASSERT_FLOAT_WITHIN(0.01F,40000,good.frequencyHz);
    TEST_ASSERT_TRUE(rainFrequencyWindow(0,250000,0,false).fault == RainFrequencyFault::NoSignal);
    TEST_ASSERT_TRUE(rainFrequencyWindow(0,250000,100,true).fault == RainFrequencyFault::CounterOverflow);
    TEST_ASSERT_TRUE(rainFrequencyWindow(0,1600000,100,false).fault == RainFrequencyFault::InvalidWindow);
    TEST_ASSERT_FALSE(rainFrequencyWindow(100,100,10,false).valid());
    TEST_ASSERT_FALSE(rainFrequencyWindow(0,1000,10,false).valid());
}
void test_acquisition_delivers_every_window_and_fault_without_publication_poll() {
    TestFrequencyInput input;
    TestFrequencyObserver observer;
    RainFrequencyAcquisition acquisition(input,observer);
    TEST_ASSERT_TRUE(acquisition.begin(27));
    acquisition.service();
    TEST_ASSERT_EQUAL_UINT(0,observer.count);
    input.next=RainFrequencyPoll::Window;
    input.window=rainFrequencyWindow(0,250000,10000,false);
    acquisition.service(); acquisition.service();
    TEST_ASSERT_EQUAL_UINT(2,observer.count);
    input.window=rainFrequencyWindow(250000,500000,0,false);
    acquisition.service();
    TEST_ASSERT_FALSE(observer.last.valid());
    TEST_ASSERT_TRUE(acquisition.running()); // Missing signal may recover next gate.
    input.window=rainFrequencyWindow(500000,750000,9000,false);
    acquisition.service();
    TEST_ASSERT_TRUE(observer.last.valid());
    RainFrequencyWindow copy;
    TEST_ASSERT_TRUE(acquisition.latest(copy));
    TEST_ASSERT_EQUAL_UINT(9000,copy.pulseCount);
    input.next=RainFrequencyPoll::Failed;
    acquisition.service();
    TEST_ASSERT_FALSE(acquisition.running());
    TEST_ASSERT_TRUE(observer.last.fault == RainFrequencyFault::DriverError);
    unsigned count=observer.count;
    acquisition.service();
    TEST_ASSERT_EQUAL_UINT(count,observer.count);
    input.startOk=false;
    TEST_ASSERT_FALSE(acquisition.begin(27));
    TEST_ASSERT_FALSE(observer.last.valid());
    input.startOk=true; input.next=RainFrequencyPoll::Pending;
    TEST_ASSERT_TRUE(acquisition.begin(27));
    TEST_ASSERT_FALSE(acquisition.latest(copy));
}
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_shared_policy_wetting_end_and_dry_are_independent);
    RUN_TEST(test_signal_loss_aborts_cycle_but_preserves_arming);
    RUN_TEST(test_config_is_atomic_and_bounded);
    RUN_TEST(test_frequency_conversion_detects_zero_wrap_and_gate_errors);
    RUN_TEST(test_acquisition_delivers_every_window_and_fault_without_publication_poll);
    return UNITY_END();
}
