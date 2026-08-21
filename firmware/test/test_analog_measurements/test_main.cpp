#include <Arduino.h>
#include <unity.h>

#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

#include "AverageAnalogSampler.h"
#include "LinearTwoPointCalibration.h"

using namespace EnvNode;

HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t length) { return length; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}

namespace {

class FakeClock : public IMonotonicClock {
public:
    uint32_t nowMs() const override { return nowMs_; }
    void set(uint32_t nowMs) { nowMs_ = nowMs; }

private:
    uint32_t nowMs_ = 0;
};

class FakeAnalogInput : public IAnalogInput {
public:
    bool begin() override {
        initialized_ = beginSucceeds;
        return beginSucceeds;
    }

    AnalogSample read() override {
        ++readCount;
        if (nextSample < samples.size()) return samples[nextSample++];
        return {defaultVoltage, true, AnalogInputError::None};
    }

    bool initialized() const override { return initialized_; }

    bool beginSucceeds = true;
    float defaultVoltage = 1.0f;
    size_t readCount = 0;
    size_t nextSample = 0;
    std::vector<AnalogSample> samples;

private:
    bool initialized_ = false;
};

void test_calibration_maps_reference_points_and_interpolates() {
    const LinearTwoPointCalibration calibration {1.0f, 10.0f, 5.0f, 30.0f};
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 10.0f,
        applyLinearTwoPointCalibration(calibration, 1.0f).value);
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 30.0f,
        applyLinearTwoPointCalibration(calibration, 5.0f).value);
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 20.0f,
        applyLinearTwoPointCalibration(calibration, 3.0f).value);
}

void test_calibration_extrapolates_and_supports_inverted_characteristic() {
    const LinearTwoPointCalibration calibration {5.0f, 0.0f, 1.0f, 100.0f};
    const LinearCalibrationResult extrapolated =
        applyLinearTwoPointCalibration(calibration, 0.0f);
    TEST_ASSERT_TRUE(extrapolated.valid);
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 125.0f, extrapolated.value);
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 50.0f,
        applyLinearTwoPointCalibration(calibration, 3.0f).value);
}

void test_calibration_rejects_equal_inputs_and_non_finite_values() {
    LinearTwoPointCalibration calibration {1.0f, 0.0f, 1.0f, 10.0f};
    TEST_ASSERT_FALSE(isValid(calibration));
    TEST_ASSERT_FALSE(applyLinearTwoPointCalibration(calibration, 1.0f).valid);

    calibration = {0.0f, 0.0f, 1.0f, 1.0f};
    TEST_ASSERT_FALSE(applyLinearTwoPointCalibration(
        calibration, std::numeric_limits<float>::quiet_NaN()).valid);
    calibration.valueAtMax = std::numeric_limits<float>::infinity();
    TEST_ASSERT_FALSE(isValid(calibration));
    TEST_ASSERT_FALSE(applyLinearTwoPointCalibration(calibration, 0.5f).valid);
}

void test_sampler_obeys_interval_and_completes_average_window() {
    FakeClock clock;
    FakeAnalogInput input;
    input.samples = {
        {1.0f, true, AnalogInputError::None},
        {2.0f, true, AnalogInputError::None},
        {3.0f, true, AnalogInputError::None},
    };
    AverageAnalogSampler sampler(input, clock, {20, 60, 3});
    TEST_ASSERT_TRUE(sampler.begin());

    sampler.service();
    clock.set(19); sampler.service();
    TEST_ASSERT_EQUAL_UINT32(1, input.readCount);
    clock.set(20); sampler.service();
    clock.set(40); sampler.service();
    clock.set(60); sampler.service();

    TEST_ASSERT_TRUE(sampler.hasCompletedWindow());
    const FilteredAnalogValue& result = sampler.latestCompletedWindow();
    TEST_ASSERT_TRUE(result.valid);
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 2.0f, result.voltage);
    TEST_ASSERT_EQUAL_UINT32(0, result.windowStartMs);
    TEST_ASSERT_EQUAL_UINT32(60, result.windowEndMs);
    TEST_ASSERT_EQUAL_UINT32(3, result.acceptedSamples);
    TEST_ASSERT_EQUAL_UINT32(0, result.rejectedSamples);
    TEST_ASSERT_EQUAL_UINT32(4, input.readCount);
}

void test_sampler_rejects_failed_and_non_finite_samples() {
    FakeClock clock;
    FakeAnalogInput input;
    input.samples = {
        {2.0f, true, AnalogInputError::None},
        {0.0f, false, AnalogInputError::ReadFailed},
        {std::numeric_limits<float>::quiet_NaN(), true, AnalogInputError::None},
    };
    AverageAnalogSampler sampler(input, clock, {10, 30, 1});
    TEST_ASSERT_TRUE(sampler.begin());
    sampler.service();
    clock.set(10); sampler.service();
    clock.set(20); sampler.service();
    clock.set(30); sampler.service();

    const FilteredAnalogValue& result = sampler.latestCompletedWindow();
    TEST_ASSERT_TRUE(result.valid);
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 2.0f, result.voltage);
    TEST_ASSERT_EQUAL_UINT32(1, result.acceptedSamples);
    TEST_ASSERT_EQUAL_UINT32(2, result.rejectedSamples);
}

void test_sampler_requires_minimum_valid_sample_count() {
    FakeClock clock;
    FakeAnalogInput input;
    AverageAnalogSampler sampler(input, clock, {20, 50, 3});
    TEST_ASSERT_TRUE(sampler.begin());
    sampler.service();
    clock.set(20); sampler.service();
    clock.set(50); sampler.service();
    TEST_ASSERT_FALSE(sampler.latestCompletedWindow().valid);
    TEST_ASSERT_EQUAL_UINT32(2, sampler.latestCompletedWindow().acceptedSamples);
}

void test_delayed_service_does_not_perform_burst_reads() {
    FakeClock clock;
    FakeAnalogInput input;
    AverageAnalogSampler sampler(input, clock, {10, 100, 1});
    TEST_ASSERT_TRUE(sampler.begin());
    sampler.service();
    clock.set(95); sampler.service();
    TEST_ASSERT_EQUAL_UINT32(2, input.readCount);
    clock.set(1000); sampler.service();
    TEST_ASSERT_EQUAL_UINT32(3, input.readCount);
    TEST_ASSERT_EQUAL_UINT32(2, sampler.latestCompletedWindow().acceptedSamples);
    TEST_ASSERT_EQUAL_UINT32(100, sampler.latestCompletedWindow().windowEndMs);
}

void test_sampler_rejects_invalid_configuration_and_input_failure() {
    FakeClock clock;
    FakeAnalogInput input;
    AverageAnalogSampler zeroInterval(input, clock, {0, 100, 1});
    TEST_ASSERT_FALSE(zeroInterval.begin());
    AverageAnalogSampler zeroWindow(input, clock, {10, 0, 1});
    TEST_ASSERT_FALSE(zeroWindow.begin());
    AverageAnalogSampler zeroMinimum(input, clock, {10, 100, 0});
    TEST_ASSERT_FALSE(zeroMinimum.begin());
    AverageAnalogSampler ambiguousDeadline(
        input, clock, {10, static_cast<uint32_t>(INT32_MAX) + 1U, 1});
    TEST_ASSERT_FALSE(ambiguousDeadline.begin());

    FakeAnalogInput failingInput;
    failingInput.beginSucceeds = false;
    AverageAnalogSampler failed(failingInput, clock, {10, 100, 1});
    TEST_ASSERT_FALSE(failed.begin());
    TEST_ASSERT_FALSE(failed.initialized());
}

void test_sampler_deadlines_work_across_clock_wraparound() {
    FakeClock clock;
    clock.set(UINT32_MAX - 20U);
    FakeAnalogInput input;
    input.defaultVoltage = 4.0f;
    AverageAnalogSampler sampler(input, clock, {10, 30, 3});
    TEST_ASSERT_TRUE(sampler.begin());
    sampler.service();
    clock.set(UINT32_MAX - 10U); sampler.service();
    clock.set(UINT32_MAX); sampler.service();
    clock.set(9); sampler.service();

    const FilteredAnalogValue& result = sampler.latestCompletedWindow();
    TEST_ASSERT_TRUE(result.valid);
    TEST_ASSERT_EQUAL_UINT32(UINT32_MAX - 20U, result.windowStartMs);
    TEST_ASSERT_EQUAL_UINT32(9, result.windowEndMs);
    TEST_ASSERT_EQUAL_UINT32(3, result.acceptedSamples);
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 4.0f, result.voltage);
}

} // namespace

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_calibration_maps_reference_points_and_interpolates);
    RUN_TEST(test_calibration_extrapolates_and_supports_inverted_characteristic);
    RUN_TEST(test_calibration_rejects_equal_inputs_and_non_finite_values);
    RUN_TEST(test_sampler_obeys_interval_and_completes_average_window);
    RUN_TEST(test_sampler_rejects_failed_and_non_finite_samples);
    RUN_TEST(test_sampler_requires_minimum_valid_sample_count);
    RUN_TEST(test_delayed_service_does_not_perform_burst_reads);
    RUN_TEST(test_sampler_rejects_invalid_configuration_and_input_failure);
    RUN_TEST(test_sampler_deadlines_work_across_clock_wraparound);
    return UNITY_END();
}
