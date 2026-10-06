#include <unity.h>
#include <cstring>
#include "PropertyWebView.h"
#include "UnitConverter.h"

#include "MeasurementSnapshotCache.h"
#include "SensorPropertyReader.h"

using namespace EnvNode;

HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t length) { return length; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}

class TestSensor : public ISensor {
public:
    SensorId id() const override { return 3; }
    const char* type() const override { return "test"; }
    SensorProvenance provenance() const override { return SensorProvenance::Simulated; }
    SensorState state() const override { return SensorState::Ready; }
    bool supports(MeasurementType type) const override {
        return type == MeasurementType::Temperature
            || type == MeasurementType::RainDetectorWet
            || type == MeasurementType::RainGaugeTip;
    }
    void begin() override { ++hardwareCalls; }
    SensorOperationResult service(IMeasurementSink&) override {
        ++hardwareCalls;
        return SensorOperationResult::NoData;
    }
    SensorOperationResult sample(IMeasurementSink&) override {
        ++hardwareCalls;
        return SensorOperationResult::NoData;
    }
    unsigned hardwareCalls = 0;
};

const PropertyReference Temperature(PropertyComponentKind::Sensor, 3, "temperature");

Measurement temperature(float value) {
    Measurement result;
    result.source = 3;
    result.type = MeasurementType::Temperature;
    result.value = MeasurementValue::floatingPoint(value);
    result.valid = true;
    result.timestamp = 123456;
    return result;
}

void test_description_exists_before_first_measurement() {
    TestSensor sensor;
    MeasurementSnapshotCache cache;
    SensorPropertyReader adapter(sensor, cache);
    const IPropertyReader& reader = adapter;
    PropertyDescription description;
    TEST_ASSERT_TRUE(reader.describe(Temperature, description));
    TEST_ASSERT_EQUAL_STRING("temperature", description.stableKey);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyValueKind::FloatingPoint), static_cast<int>(description.valueKind));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PresentationUnit::DegreeCelsius), static_cast<int>(description.canonicalUnit));
    PropertySnapshot result;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyReadResult::NoValue), static_cast<int>(reader.read(Temperature, result)));
    TEST_ASSERT_FALSE(result.valid);
    TEST_ASSERT_EQUAL_UINT32(0, sensor.hardwareCalls);
}

void test_temperature_read_preserves_value_quality_and_times() {
    TestSensor sensor;
    MeasurementSnapshotCache cache;
    SensorPropertyReader reader(sensor, cache);
    Measurement input = temperature(21.5F);
    input.quality = MeasurementQuality::Estimated;
    cache.observe(input, 4321);
    MeasurementSnapshot original;
    cache.latest(MeasurementSourceReference(3, MeasurementType::Temperature), original);
    PropertySnapshot result;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyReadResult::Available), static_cast<int>(reader.read(Temperature, result)));
    float value = 0;
    TEST_ASSERT_TRUE(result.value.tryGetFloatingPoint(value));
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 21.5F, value);
    TEST_ASSERT_TRUE(result.valid);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(input.quality), static_cast<int>(result.quality));
    TEST_ASSERT_EQUAL_INT64(input.timestamp, result.timestamp);
    TEST_ASSERT_EQUAL_UINT32(4321, result.acceptedMonotonicMs);
    TEST_ASSERT_EQUAL_UINT32(original.revision, result.revision);
    TEST_ASSERT_TRUE(result.hasQuality);
    TEST_ASSERT_TRUE(result.hasTimestamp);
    TEST_ASSERT_TRUE(result.hasAcceptedMonotonicMs);
    TEST_ASSERT_TRUE(result.hasRevision);
    TEST_ASSERT_EQUAL_UINT32(0, sensor.hardwareCalls);
}

void test_unknown_references_and_unsupported_measurements_clear_outputs() {
    TestSensor sensor;
    MeasurementSnapshotCache cache;
    SensorPropertyReader reader(sensor, cache);
    cache.observe(temperature(21.5F), 10);
    const PropertyReference references[] = {
        {PropertyComponentKind::Unknown, 3, "temperature"},
        {PropertyComponentKind::Sensor, 0, "temperature"},
        {PropertyComponentKind::Sensor, 4, "temperature"},
        {PropertyComponentKind::Sensor, 3, nullptr},
        {PropertyComponentKind::Sensor, 3, ""},
        {PropertyComponentKind::Sensor, 3, "missing"},
        {PropertyComponentKind::Sensor, 3, "Temperature"},
        {PropertyComponentKind::Sensor, 3, "relative_humidity"},
    };
    for (const PropertyReference& reference : references) {
        PropertyDescription description;
        TEST_ASSERT_TRUE(reader.describe(Temperature, description));
        TEST_ASSERT_FALSE(reader.describe(reference, description));
        TEST_ASSERT_EQUAL_STRING("", description.stableKey);
        PropertySnapshot result;
        reader.read(Temperature, result);
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyReadResult::UnknownReference), static_cast<int>(reader.read(reference, result)));
        TEST_ASSERT_FALSE(result.valid);
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyValueKind::None), static_cast<int>(result.value.kind()));
        TEST_ASSERT_EQUAL_UINT32(0, result.revision);
    }
}

void test_invalid_measurement_is_available_but_invalid() {
    TestSensor sensor;
    MeasurementSnapshotCache cache;
    SensorPropertyReader reader(sensor, cache);
    Measurement input = temperature(0);
    input.valid = false;
    input.value = MeasurementValue::none();
    input.quality = MeasurementQuality::Degraded;
    cache.observe(input, 500);
    PropertySnapshot result;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyReadResult::Available), static_cast<int>(reader.read(Temperature, result)));
    TEST_ASSERT_FALSE(result.valid);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyValueKind::None), static_cast<int>(result.value.kind()));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(MeasurementQuality::Degraded), static_cast<int>(result.quality));
    TEST_ASSERT_EQUAL_UINT32(500, result.acceptedMonotonicMs);
    TEST_ASSERT_EQUAL_INT64(input.timestamp, result.timestamp);
}

void test_updates_and_clear_are_read_through_without_second_cache() {
    TestSensor sensor;
    MeasurementSnapshotCache cache;
    SensorPropertyReader reader(sensor, cache);
    cache.observe(temperature(21.5F), 10);
    PropertySnapshot first;
    reader.read(Temperature, first);
    first.value = MeasurementValue::floatingPoint(99);
    PropertySnapshot result;
    reader.read(Temperature, result);
    float value = 0;
    result.value.tryGetFloatingPoint(value);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 21.5F, value);
    cache.observe(temperature(22.5F), 20);
    reader.read(Temperature, result);
    result.value.tryGetFloatingPoint(value);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 22.5F, value);
    TEST_ASSERT_NOT_EQUAL(first.revision, result.revision);
    cache.clear();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyReadResult::NoValue), static_cast<int>(reader.read(Temperature, result)));
    TEST_ASSERT_FALSE(result.valid);
    TEST_ASSERT_EQUAL_UINT32(0, result.acceptedMonotonicMs);
    TEST_ASSERT_EQUAL_UINT32(0, result.revision);
}

void test_boolean_state_preserves_type_and_rejects_float_access() {
    TestSensor sensor;
    MeasurementSnapshotCache cache;
    SensorPropertyReader reader(sensor, cache);
    Measurement input = temperature(0);
    input.type = MeasurementType::RainDetectorWet;
    input.value = MeasurementValue::boolean(true);
    cache.observe(input, 10);
    const PropertyReference wet(PropertyComponentKind::Sensor, 3, "rain_detector_wet");
    PropertyDescription description;
    TEST_ASSERT_TRUE(reader.describe(wet, description));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyValueKind::Boolean), static_cast<int>(description.valueKind));
    PropertySnapshot result;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyReadResult::Available), static_cast<int>(reader.read(wet, result)));
    bool value = false;
    float wrongType = 0;
    TEST_ASSERT_TRUE(result.value.tryGetBoolean(value));
    TEST_ASSERT_TRUE(value);
    TEST_ASSERT_FALSE(result.value.tryGetFloatingPoint(wrongType));
}

void test_events_are_not_exposed_as_state_properties() {
    TestSensor sensor;
    MeasurementSnapshotCache cache;
    SensorPropertyReader reader(sensor, cache);
    const PropertyReference event(PropertyComponentKind::Sensor, 3, "rain_gauge_tip");
    PropertyDescription description;
    PropertySnapshot result;
    TEST_ASSERT_FALSE(reader.describe(event, description));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyReadResult::UnknownReference), static_cast<int>(reader.read(event, result)));
}

void test_web_diagnostic_reads_current_value_and_age_through_adapter() {
    TestSensor sensor;
    MeasurementSnapshotCache cache;
    SensorPropertyReader reader(sensor, cache);
    cache.observe(temperature(21.5F), 1000);
    String html = buildPropertyDiagnosticHtml(reader, Temperature, 6000);
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "sensor / 3 / temperature"));
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "21.50"));
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), UnitConverter::symbol(PresentationUnit::DegreeCelsius)));
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "<dt>Validity</dt><dd>Valid"));
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "<dt>Age</dt><dd>5 s"));
    cache.observe(temperature(22.5F), 7000);
    html = buildPropertyDiagnosticHtml(reader, Temperature, 8000);
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "22.50"));
    TEST_ASSERT_NULL(strstr(html.c_str(), "21.50"));
    TEST_ASSERT_EQUAL_UINT32(0, sensor.hardwareCalls);
}

void test_web_diagnostic_distinguishes_missing_and_invalid_values() {
    TestSensor sensor;
    MeasurementSnapshotCache cache;
    SensorPropertyReader reader(sensor, cache);
    String html = buildPropertyDiagnosticHtml(reader, Temperature, 5000);
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "No measurement available yet"));
    Measurement input = temperature(99);
    input.valid = false;
    input.quality = MeasurementQuality::Degraded;
    cache.observe(input, 1000);
    html = buildPropertyDiagnosticHtml(reader, Temperature, 5000);
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "<dt>Validity</dt><dd>Invalid"));
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), measurementQualityDisplayName(input.quality)));
    TEST_ASSERT_NULL(strstr(html.c_str(), "99.00"));
    TEST_ASSERT_NULL(strstr(html.c_str(), "No measurement available yet"));
}

void test_web_diagnostic_escapes_unknown_reference_and_handles_clock_wrap() {
    TestSensor sensor;
    MeasurementSnapshotCache cache;
    SensorPropertyReader reader(sensor, cache);
    const PropertyReference unknown(PropertyComponentKind::Sensor, 3, "<script>");
    String html = buildPropertyDiagnosticHtml(reader, unknown, 0);
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "Unknown reference"));
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "&lt;script&gt;"));
    TEST_ASSERT_NULL(strstr(html.c_str(), "<script>"));
    cache.observe(temperature(21), UINT32_MAX - 999);
    html = buildPropertyDiagnosticHtml(reader, Temperature, 1000);
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "<dt>Age</dt><dd>2 s"));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_description_exists_before_first_measurement);
    RUN_TEST(test_temperature_read_preserves_value_quality_and_times);
    RUN_TEST(test_unknown_references_and_unsupported_measurements_clear_outputs);
    RUN_TEST(test_invalid_measurement_is_available_but_invalid);
    RUN_TEST(test_updates_and_clear_are_read_through_without_second_cache);
    RUN_TEST(test_boolean_state_preserves_type_and_rejects_float_access);
    RUN_TEST(test_events_are_not_exposed_as_state_properties);
    RUN_TEST(test_web_diagnostic_reads_current_value_and_age_through_adapter);
    RUN_TEST(test_web_diagnostic_distinguishes_missing_and_invalid_values);
    RUN_TEST(test_web_diagnostic_escapes_unknown_reference_and_handles_clock_wrap);
    return UNITY_END();
}
