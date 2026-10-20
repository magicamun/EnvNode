// EnvNode-owned: see ../../../LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
#include <unity.h>
#include <Arduino.h>
#include <RainDetectorPolicy.h>
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
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_shared_policy_wetting_end_and_dry_are_independent);
    RUN_TEST(test_signal_loss_aborts_cycle_but_preserves_arming);
    RUN_TEST(test_config_is_atomic_and_bounded);
    return UNITY_END();
}
