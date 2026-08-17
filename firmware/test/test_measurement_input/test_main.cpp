#include <unity.h>

#include "IMeasurementResolver.h"
#include "MeasurementSnapshotCache.h"
#include "MeasurementSourceReference.h"

using namespace EnvNode;

HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t length) { return length; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}

Measurement measurement(
    SensorId sensorId,
    MeasurementType type,
    float value,
    bool valid = true,
    MeasurementQuality quality = MeasurementQuality::Good) {
    Measurement result;
    result.source = sensorId;
    result.type = type;
    result.timestamp = 1000;
    result.valid = valid;
    result.quality = quality;
    result.value = valid
        ? MeasurementValue::floatingPoint(value)
        : MeasurementValue::none();
    return result;
}

void test_measurement_source_reference_equality_uses_sensor_and_type() {
    const MeasurementSourceReference humidity(4, MeasurementType::RelativeHumidity);
    TEST_ASSERT_TRUE(humidity == MeasurementSourceReference(
        4, MeasurementType::RelativeHumidity));
    TEST_ASSERT_TRUE(humidity != MeasurementSourceReference(
        5, MeasurementType::RelativeHumidity));
    TEST_ASSERT_TRUE(humidity != MeasurementSourceReference(
        4, MeasurementType::Temperature));
}

void test_resolver_lookup_uses_sensor_and_measurement_type() {
    MeasurementSnapshotCache cache;
    IMeasurementResolver& resolver = cache;
    cache.observe(measurement(3, MeasurementType::Temperature, 21.5F), 10);
    cache.observe(measurement(3, MeasurementType::RelativeHumidity, 64.0F), 11);

    MeasurementSnapshot temperature;
    MeasurementSnapshot humidity;
    TEST_ASSERT_TRUE(resolver.latest(
        MeasurementSourceReference(3, MeasurementType::Temperature), temperature));
    TEST_ASSERT_TRUE(resolver.latest(
        MeasurementSourceReference(3, MeasurementType::RelativeHumidity), humidity));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(MeasurementType::Temperature),
        static_cast<int>(temperature.measurement.type));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(MeasurementType::RelativeHumidity),
        static_cast<int>(humidity.measurement.type));
    TEST_ASSERT_NOT_EQUAL(temperature.revision, humidity.revision);
    TEST_ASSERT_FALSE(resolver.latest(
        MeasurementSourceReference(4, MeasurementType::Temperature), temperature));
}

void test_latest_update_replaces_snapshot_and_preserves_monotonic_time() {
    MeasurementSnapshotCache cache;
    const MeasurementSourceReference source(1, MeasurementType::Temperature);
    cache.observe(measurement(1, MeasurementType::Temperature, 10.0F), 123);
    MeasurementSnapshot first;
    TEST_ASSERT_TRUE(cache.latest(source, first));
    cache.observe(measurement(1, MeasurementType::Temperature, 20.0F), 456);
    MeasurementSnapshot second;
    TEST_ASSERT_TRUE(cache.latest(source, second));
    float value = 0.0F;
    TEST_ASSERT_TRUE(second.measurement.value.tryGetFloatingPoint(value));
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 20.0F, value);
    TEST_ASSERT_EQUAL_UINT32(456, second.acceptedMonotonicMs);
    TEST_ASSERT_NOT_EQUAL(first.revision, second.revision);
}

void test_same_monotonic_time_still_produces_distinct_revisions() {
    MeasurementSnapshotCache cache;
    const MeasurementSourceReference source(1, MeasurementType::Temperature);
    cache.observe(measurement(1, MeasurementType::Temperature, 10.0F), 100);
    MeasurementSnapshot first;
    cache.latest(source, first);
    cache.observe(measurement(1, MeasurementType::Temperature, 11.0F), 100);
    MeasurementSnapshot second;
    cache.latest(source, second);
    TEST_ASSERT_EQUAL_UINT32(first.acceptedMonotonicMs, second.acceptedMonotonicMs);
    TEST_ASSERT_NOT_EQUAL(first.revision, second.revision);
}

void test_clear_invalidates_every_snapshot_and_revision_sequence_continues() {
    MeasurementSnapshotCache cache;
    const MeasurementSourceReference temperature(1, MeasurementType::Temperature);
    const MeasurementSourceReference humidity(2, MeasurementType::RelativeHumidity);
    cache.observe(measurement(1, MeasurementType::Temperature, 10.0F), 100);
    cache.observe(measurement(2, MeasurementType::RelativeHumidity, 50.0F), 100);
    MeasurementSnapshot beforeClear;
    cache.latest(temperature, beforeClear);

    cache.clear();
    MeasurementSnapshot result;
    TEST_ASSERT_FALSE(cache.latest(temperature, result));
    TEST_ASSERT_FALSE(cache.latest(humidity, result));

    cache.observe(measurement(1, MeasurementType::Temperature, 12.0F), 100);
    TEST_ASSERT_TRUE(cache.latest(temperature, result));
    TEST_ASSERT_NOT_EQUAL(beforeClear.revision, result.revision);
}

void test_invalid_measurement_and_quality_are_preserved_without_policy() {
    MeasurementSnapshotCache cache;
    Measurement invalid = measurement(
        1, MeasurementType::Temperature, 0.0F, false,
        MeasurementQuality::Degraded);
    cache.observe(invalid, 200);
    MeasurementSnapshot result;
    TEST_ASSERT_TRUE(cache.latest(
        MeasurementSourceReference(1, MeasurementType::Temperature), result));
    TEST_ASSERT_FALSE(result.measurement.valid);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(MeasurementQuality::Degraded),
        static_cast<int>(result.measurement.quality));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ValueKind::None),
        static_cast<int>(result.measurement.value.kind()));
}

void test_value_kind_and_measurement_semantics_remain_uninterpreted() {
    MeasurementSnapshotCache cache;
    Measurement wet;
    wet.source = 2;
    wet.type = MeasurementType::RainDetectorWet;
    wet.timestamp = 1000;
    wet.valid = true;
    wet.value = MeasurementValue::boolean(true);
    cache.observe(wet, 300);
    MeasurementSnapshot result;
    TEST_ASSERT_TRUE(cache.latest(
        MeasurementSourceReference(2, MeasurementType::RainDetectorWet), result));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ValueKind::Boolean),
        static_cast<int>(result.measurement.value.kind()));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(MeasurementSemantics::State),
        static_cast<int>(measurementTypeMetadata(result.measurement.type).semantics));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(MeasurementSemantics::Event),
        static_cast<int>(measurementTypeMetadata(MeasurementType::RainGaugeTip).semantics));
}

void test_resolver_returns_a_copy_not_mutable_cache_storage() {
    MeasurementSnapshotCache cache;
    const MeasurementSourceReference source(1, MeasurementType::Temperature);
    cache.observe(measurement(1, MeasurementType::Temperature, 15.0F), 400);
    MeasurementSnapshot first;
    cache.latest(source, first);
    first.measurement.value = MeasurementValue::floatingPoint(99.0F);
    first.acceptedMonotonicMs = 999;
    first.revision = 999;

    MeasurementSnapshot second;
    cache.latest(source, second);
    float value = 0.0F;
    TEST_ASSERT_TRUE(second.measurement.value.tryGetFloatingPoint(value));
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 15.0F, value);
    TEST_ASSERT_EQUAL_UINT32(400, second.acceptedMonotonicMs);
    TEST_ASSERT_NOT_EQUAL_UINT32(999, second.revision);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_measurement_source_reference_equality_uses_sensor_and_type);
    RUN_TEST(test_resolver_lookup_uses_sensor_and_measurement_type);
    RUN_TEST(test_latest_update_replaces_snapshot_and_preserves_monotonic_time);
    RUN_TEST(test_same_monotonic_time_still_produces_distinct_revisions);
    RUN_TEST(test_clear_invalidates_every_snapshot_and_revision_sequence_continues);
    RUN_TEST(test_invalid_measurement_and_quality_are_preserved_without_policy);
    RUN_TEST(test_value_kind_and_measurement_semantics_remain_uninterpreted);
    RUN_TEST(test_resolver_returns_a_copy_not_mutable_cache_storage);
    return UNITY_END();
}
