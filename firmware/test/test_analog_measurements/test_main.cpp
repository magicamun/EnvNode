#include <Arduino.h>
#include <unity.h>

#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

#include "AverageAnalogSampler.h"
#include "AnalogPressureSensor.h"
#include "LinearTwoPointCalibration.h"
#include "MqttTopic.h"
#include "UnitConverter.h"

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

class CollectingSink : public IMeasurementSink {
public:
    void emit(const Measurement& measurement) override { measurements.push_back(measurement); }
    std::vector<Measurement> measurements;
};

AnalogPressureSensorConfiguration pressureConfiguration(bool waterLevelEnabled = true) {
    return {0.5f, 4.5f, {1.0f, 0.0f, 4.0f, 30000.0f}, 1000.0f,
        waterLevelEnabled, {1000.0f, 10.0f}};
}

void completeWindow(AverageAnalogSampler& sampler, FakeClock& clock, uint32_t endMs = 10) {
    sampler.service();
    clock.set(endMs);
    sampler.service();
}

float measurementFloat(const Measurement& measurement) {
    float value = 0.0f;
    TEST_ASSERT_TRUE(measurement.value.tryGetFloatingPoint(value));
    return value;
}

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

void test_pressure_sensor_initializes_and_reports_capabilities() {
    FakeClock clock;
    FakeAnalogInput input;
    AverageAnalogSampler sampler(input, clock, {10, 10, 1});
    AnalogPressureSensor sensor(7, sampler, pressureConfiguration());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SensorState::Unknown), static_cast<int>(sensor.state()));
    sensor.begin();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SensorState::Ready), static_cast<int>(sensor.state()));
    TEST_ASSERT_TRUE(sensor.supports(MeasurementType::HydrostaticPressure));
    TEST_ASSERT_TRUE(sensor.supports(MeasurementType::WaterLevel));
    TEST_ASSERT_FALSE(sensor.supports(MeasurementType::AtmosphericPressure));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SensorProvenance::Physical),
        static_cast<int>(sensor.provenance()));
}

void test_pressure_sensor_rejects_invalid_configurations() {
    FakeClock clock;
    FakeAnalogInput input;
    AnalogPressureSensorConfiguration configurations[] = {
        pressureConfiguration(), pressureConfiguration(), pressureConfiguration(),
        pressureConfiguration(), pressureConfiguration(), pressureConfiguration(),
    };
    configurations[0].validInputMinVoltage = configurations[0].validInputMaxVoltage;
    configurations[1].validInputMinVoltage = std::numeric_limits<float>::quiet_NaN();
    configurations[2].pressureCalibration.inputAtMax = configurations[2].pressureCalibration.inputAtMin;
    configurations[3].minimumReliablePressurePascal = std::numeric_limits<float>::infinity();
    configurations[4].waterLevel.liquidDensityKgPerCubicMetre = 0.0f;
    configurations[5].waterLevel.gravitationalAccelerationMetresPerSecondSquared = -1.0f;
    for (const auto& configuration : configurations) {
        AverageAnalogSampler sampler(input, clock, {10, 10, 1});
        AnalogPressureSensor sensor(1, sampler, configuration);
        sensor.begin();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(SensorState::Failed), static_cast<int>(sensor.state()));
    }
}

void test_disabled_water_level_ignores_level_configuration_and_emits_pressure_only() {
    FakeClock clock;
    FakeAnalogInput input;
    input.defaultVoltage = 2.0f;
    AverageAnalogSampler sampler(input, clock, {10, 10, 1});
    auto configuration = pressureConfiguration(false);
    configuration.waterLevel = {0.0f, std::numeric_limits<float>::quiet_NaN()};
    AnalogPressureSensor sensor(1, sampler, configuration);
    CollectingSink sink;
    sensor.begin();
    completeWindow(sampler, clock);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SensorOperationResult::Completed),
        static_cast<int>(sensor.sample(sink)));
    TEST_ASSERT_EQUAL_UINT32(1, sink.measurements.size());
    TEST_ASSERT_FALSE(sensor.supports(MeasurementType::WaterLevel));
}

void test_no_completed_window_returns_no_data() {
    FakeClock clock;
    FakeAnalogInput input;
    AverageAnalogSampler sampler(input, clock, {10, 10, 1});
    AnalogPressureSensor sensor(1, sampler, pressureConfiguration());
    CollectingSink sink;
    sensor.begin();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SensorOperationResult::NoData),
        static_cast<int>(sensor.sample(sink)));
    TEST_ASSERT_TRUE(sink.measurements.empty());
}

void test_normal_pressure_and_water_level_are_emitted() {
    FakeClock clock;
    FakeAnalogInput input;
    input.defaultVoltage = 2.0f;
    AverageAnalogSampler sampler(input, clock, {10, 10, 1});
    AnalogPressureSensor sensor(1, sampler, pressureConfiguration());
    CollectingSink sink;
    sensor.begin();
    completeWindow(sampler, clock);
    sensor.sample(sink);
    TEST_ASSERT_EQUAL_UINT32(2, sink.measurements.size());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(MeasurementType::HydrostaticPressure),
        static_cast<int>(sink.measurements[0].type));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10000.0f, measurementFloat(sink.measurements[0]));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, measurementFloat(sink.measurements[1]));
    TEST_ASSERT_TRUE(sink.measurements[0].valid);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(sink.measurements[0].quality),
        static_cast<int>(sink.measurements[1].quality));
}

void test_electrical_boundaries_are_inclusive_and_outside_values_invalid() {
    const float voltages[] = {0.5f, 4.5f, 0.499f, 4.501f};
    const bool expectedValid[] = {true, true, false, false};
    for (size_t index = 0; index < 4; ++index) {
        FakeClock clock;
        FakeAnalogInput input;
        input.defaultVoltage = voltages[index];
        AverageAnalogSampler sampler(input, clock, {10, 10, 1});
        AnalogPressureSensor sensor(1, sampler, pressureConfiguration(false));
        CollectingSink sink;
        sensor.begin();
        completeWindow(sampler, clock);
        sensor.sample(sink);
        TEST_ASSERT_EQUAL(expectedValid[index], sink.measurements[0].valid);
        if (!expectedValid[index]) {
            TEST_ASSERT_EQUAL_INT(static_cast<int>(ValueKind::None),
                static_cast<int>(sink.measurements[0].value.kind()));
            TEST_ASSERT_EQUAL_INT(static_cast<int>(MeasurementQuality::Degraded),
                static_cast<int>(sink.measurements[0].quality));
        }
    }
}

void test_calibration_extrapolates_within_electrical_range() {
    FakeClock clock;
    FakeAnalogInput input;
    input.defaultVoltage = 0.5f;
    AverageAnalogSampler sampler(input, clock, {10, 10, 1});
    AnalogPressureSensor sensor(1, sampler, pressureConfiguration(false));
    CollectingSink sink;
    sensor.begin(); completeWindow(sampler, clock); sensor.sample(sink);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -5000.0f, measurementFloat(sink.measurements[0]));
}

void test_reliable_pressure_threshold_controls_quality_inclusively() {
    const float voltages[] = {1.099f, 1.1f};
    const MeasurementQuality qualities[] = {
        MeasurementQuality::BelowMeasurementRange, MeasurementQuality::Good};
    for (size_t index = 0; index < 2; ++index) {
        FakeClock clock; FakeAnalogInput input; input.defaultVoltage = voltages[index];
        AverageAnalogSampler sampler(input, clock, {10, 10, 1});
        AnalogPressureSensor sensor(1, sampler, pressureConfiguration());
        CollectingSink sink;
        sensor.begin(); completeWindow(sampler, clock); sensor.sample(sink);
        TEST_ASSERT_TRUE(sink.measurements[0].valid);
        TEST_ASSERT_EQUAL_INT(static_cast<int>(qualities[index]),
            static_cast<int>(sink.measurements[0].quality));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(sink.measurements[0].quality),
            static_cast<int>(sink.measurements[1].quality));
    }
}

void test_invalid_completed_window_emits_invalid_measurements_and_completed() {
    FakeClock clock;
    FakeAnalogInput input;
    AverageAnalogSampler sampler(input, clock, {10, 10, 2});
    AnalogPressureSensor sensor(1, sampler, pressureConfiguration());
    CollectingSink sink;
    sensor.begin(); completeWindow(sampler, clock);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SensorOperationResult::Completed),
        static_cast<int>(sensor.sample(sink)));
    TEST_ASSERT_EQUAL_UINT32(2, sink.measurements.size());
    TEST_ASSERT_FALSE(sink.measurements[0].valid);
    TEST_ASSERT_FALSE(sink.measurements[1].valid);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(sink.measurements[0].quality),
        static_cast<int>(sink.measurements[1].quality));
}

void test_repeated_sample_republishes_latest_completed_window() {
    FakeClock clock; FakeAnalogInput input;
    AverageAnalogSampler sampler(input, clock, {10, 10, 1});
    AnalogPressureSensor sensor(1, sampler, pressureConfiguration(false));
    CollectingSink sink;
    sensor.begin(); completeWindow(sampler, clock);
    sensor.sample(sink); sensor.sample(sink);
    TEST_ASSERT_EQUAL_UINT32(2, sink.measurements.size());
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, measurementFloat(sink.measurements[0]),
        measurementFloat(sink.measurements[1]));
}

void test_sampler_initialization_failure_fails_sensor() {
    FakeClock clock; FakeAnalogInput input; input.beginSucceeds = false;
    AverageAnalogSampler sampler(input, clock, {10, 10, 1});
    AnalogPressureSensor sensor(1, sampler, pressureConfiguration());
    sensor.begin();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SensorState::Failed), static_cast<int>(sensor.state()));
}

void test_new_measurement_metadata_ids_and_structural_validation() {
    TEST_ASSERT_EQUAL_UINT8(9, static_cast<uint8_t>(MeasurementType::RainfallIncrement));
    TEST_ASSERT_EQUAL_UINT8(10, static_cast<uint8_t>(MeasurementType::HydrostaticPressure));
    TEST_ASSERT_EQUAL_UINT8(11, static_cast<uint8_t>(MeasurementType::WaterLevel));
    TEST_ASSERT_EQUAL_UINT8(11, SupportedMeasurementTypeCount);
    TEST_ASSERT_EQUAL_STRING("hydrostatic_pressure",
        measurementTypeStableId(MeasurementType::HydrostaticPressure));
    TEST_ASSERT_EQUAL_STRING("Water Level", measurementTypeMetadata(MeasurementType::WaterLevel).displayName);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(MeasurementType::WaterLevel),
        static_cast<int>(measurementTypeFromStableId("water_level")));
    TEST_ASSERT_EQUAL_STRING("hydrostatic_pressure",
        mqttMeasurementTypeTopic(MeasurementType::HydrostaticPressure));

    Measurement measurement;
    measurement.type = MeasurementType::HydrostaticPressure;
    measurement.value = MeasurementValue::floatingPoint(12.0f);
    measurement.valid = true;
    TEST_ASSERT_TRUE(isMeasurementContentStructurallyValid(measurement));
    measurement.type = MeasurementType::WaterLevel;
    TEST_ASSERT_TRUE(isMeasurementContentStructurallyValid(measurement));
    measurement.value = MeasurementValue::boolean(true);
    TEST_ASSERT_FALSE(isMeasurementContentStructurallyValid(measurement));
}

void test_new_presentation_units_convert_parse_and_describe() {
    float result = 0.0f;
    TEST_ASSERT_TRUE(UnitConverter::convert(MeasurementType::HydrostaticPressure,
        12345.0f, PresentationUnit::Hectopascal, result));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 123.45f, result);
    TEST_ASSERT_TRUE(UnitConverter::convert(MeasurementType::HydrostaticPressure,
        12345.0f, PresentationUnit::Kilopascal, result));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 12.345f, result);
    TEST_ASSERT_FALSE(supportsPresentationUnit(MeasurementType::HydrostaticPressure,
        PresentationUnit::InchMercury));
    TEST_ASSERT_TRUE(UnitConverter::convert(MeasurementType::WaterLevel,
        1.234f, PresentationUnit::Centimetre, result));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 123.4f, result);
    TEST_ASSERT_TRUE(UnitConverter::convert(MeasurementType::WaterLevel,
        1.234f, PresentationUnit::Millimeter, result));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1234.0f, result);
    TEST_ASSERT_EQUAL_STRING("cm", UnitConverter::symbol(PresentationUnit::Centimetre));
    TEST_ASSERT_EQUAL_STRING("metre", UnitConverter::stableKey(PresentationUnit::Metre));
    PresentationUnit parsed = PresentationUnit::None;
    TEST_ASSERT_TRUE(UnitConverter::parseStableKey("centimetre", parsed));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PresentationUnit::Centimetre), static_cast<int>(parsed));
}

void test_below_measurement_range_quality_mappings_are_stable() {
    TEST_ASSERT_EQUAL_STRING("below_measurement_range",
        measurementQualityStableId(MeasurementQuality::BelowMeasurementRange));
    TEST_ASSERT_EQUAL_STRING("Below measurement range",
        measurementQualityDisplayName(MeasurementQuality::BelowMeasurementRange));
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
    RUN_TEST(test_pressure_sensor_initializes_and_reports_capabilities);
    RUN_TEST(test_pressure_sensor_rejects_invalid_configurations);
    RUN_TEST(test_disabled_water_level_ignores_level_configuration_and_emits_pressure_only);
    RUN_TEST(test_no_completed_window_returns_no_data);
    RUN_TEST(test_normal_pressure_and_water_level_are_emitted);
    RUN_TEST(test_electrical_boundaries_are_inclusive_and_outside_values_invalid);
    RUN_TEST(test_calibration_extrapolates_within_electrical_range);
    RUN_TEST(test_reliable_pressure_threshold_controls_quality_inclusively);
    RUN_TEST(test_invalid_completed_window_emits_invalid_measurements_and_completed);
    RUN_TEST(test_repeated_sample_republishes_latest_completed_window);
    RUN_TEST(test_sampler_initialization_failure_fails_sensor);
    RUN_TEST(test_new_measurement_metadata_ids_and_structural_validation);
    RUN_TEST(test_new_presentation_units_convert_parse_and_describe);
    RUN_TEST(test_below_measurement_range_quality_mappings_are_stable);
    return UNITY_END();
}
