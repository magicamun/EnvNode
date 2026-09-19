#include <cmath>
#include <cstring>

#include <Arduino.h>
#include <unity.h>

#include "CompactCborCodec.h"
#include "HardwareDescriptorVocabulary.h"

using namespace EnvNode;

HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
size_t HardwareSerial::write(const uint8_t*, size_t length) { return length; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}

void test_writer_matches_deterministic_cbor_vector() {
    uint8_t encoded[128];
    CompactCborWriter writer(encoded, sizeof(encoded));
    TEST_ASSERT_TRUE(writer.beginMap(5));
    TEST_ASSERT_TRUE(writer.writeUnsigned(0));
    TEST_ASSERT_TRUE(writer.beginArray(3));
    TEST_ASSERT_TRUE(writer.writeUnsigned(0));
    TEST_ASSERT_TRUE(writer.writeUnsigned(1));
    TEST_ASSERT_TRUE(writer.writeUnsigned(24));
    TEST_ASSERT_TRUE(writer.writeUnsigned(1));
    TEST_ASSERT_TRUE(writer.writeText("module"));
    TEST_ASSERT_TRUE(writer.writeUnsigned(2));
    TEST_ASSERT_TRUE(writer.writeInteger(-25));
    TEST_ASSERT_TRUE(writer.writeUnsigned(3));
    TEST_ASSERT_TRUE(writer.writeBoolean(true));
    TEST_ASSERT_TRUE(writer.writeUnsigned(4));
    TEST_ASSERT_TRUE(writer.writeNull());
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(CompactCborStatus::Success),
        static_cast<int>(writer.status()));

    const uint8_t expected[] = {
        0xA5,
        0x00, 0x83, 0x00, 0x01, 0x18, 0x18,
        0x01, 0x66, 'm', 'o', 'd', 'u', 'l', 'e',
        0x02, 0x38, 0x18,
        0x03, 0xF5,
        0x04, 0xF6,
    };
    TEST_ASSERT_EQUAL_UINT32(sizeof(expected), writer.size());
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, encoded, sizeof(expected));
}

void test_reader_round_trips_bounded_views_and_double() {
    uint8_t encoded[128];
    const uint8_t uuid[] = {0, 1, 2, 3};
    CompactCborWriter writer(encoded, sizeof(encoded));
    writer.beginArray(6);
    writer.writeUnsigned(UINT32_MAX);
    writer.writeInteger(INT32_MIN);
    writer.writeByteString(uuid, sizeof(uuid));
    writer.writeText("gpio");
    writer.writeDouble(4.25);
    writer.writeBoolean(false);

    CompactCborReader reader(encoded, writer.size());
    size_t count = 0;
    TEST_ASSERT_TRUE(reader.enterArray(count));
    TEST_ASSERT_EQUAL_UINT32(6, count);
    uint64_t unsignedValue = 0;
    int64_t signedValue = 0;
    TEST_ASSERT_TRUE(reader.readUnsigned(unsignedValue));
    TEST_ASSERT_EQUAL_UINT64(UINT32_MAX, unsignedValue);
    TEST_ASSERT_TRUE(reader.readInteger(signedValue));
    TEST_ASSERT_EQUAL_INT64(INT32_MIN, signedValue);
    const uint8_t* bytes = nullptr;
    size_t byteCount = 0;
    TEST_ASSERT_TRUE(reader.readByteString(bytes, byteCount));
    TEST_ASSERT_EQUAL_UINT32(sizeof(uuid), byteCount);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(uuid, bytes, sizeof(uuid));
    const char* text = nullptr;
    size_t textSize = 0;
    TEST_ASSERT_TRUE(reader.readText(text, textSize));
    TEST_ASSERT_EQUAL_UINT32(4, textSize);
    TEST_ASSERT_EQUAL_MEMORY("gpio", text, textSize);
    double doubleValue = 0;
    TEST_ASSERT_TRUE(reader.readDouble(doubleValue));
    TEST_ASSERT_TRUE(std::fabs(doubleValue - 4.25) < 0.000001);
    bool booleanValue = true;
    TEST_ASSERT_TRUE(reader.readBoolean(booleanValue));
    TEST_ASSERT_FALSE(booleanValue);
    TEST_ASSERT_EQUAL_UINT32(0, reader.remaining());
}

void test_reader_skips_unknown_nested_definite_values() {
    const uint8_t encoded[] = {
        0xA2,
        0x00, 0x01,
        0x18, 0x63,
        0x82,
        0xA1, 0x01, 0x62, 'o', 'k',
        0x44, 0x01, 0x02, 0x03, 0x04,
    };
    CompactCborReader reader(encoded, sizeof(encoded));
    size_t pairs = 0;
    uint64_t key = 0;
    uint64_t value = 0;
    TEST_ASSERT_TRUE(reader.enterMap(pairs));
    TEST_ASSERT_EQUAL_UINT32(2, pairs);
    TEST_ASSERT_TRUE(reader.readUnsigned(key));
    TEST_ASSERT_TRUE(reader.readUnsigned(value));
    TEST_ASSERT_EQUAL_UINT64(1, value);
    TEST_ASSERT_TRUE(reader.readUnsigned(key));
    TEST_ASSERT_EQUAL_UINT64(99, key);
    TEST_ASSERT_TRUE(reader.skip());
    TEST_ASSERT_EQUAL_UINT32(0, reader.remaining());
}

void test_noncanonical_and_indefinite_encodings_are_rejected() {
    const uint8_t nonCanonical[] = {0x18, 0x01};
    CompactCborReader first(nonCanonical, sizeof(nonCanonical));
    uint64_t value = 0;
    TEST_ASSERT_FALSE(first.readUnsigned(value));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(CompactCborStatus::NonCanonical),
        static_cast<int>(first.status()));

    const uint8_t indefiniteArray[] = {0x9F, 0x01, 0xFF};
    CompactCborReader second(indefiniteArray, sizeof(indefiniteArray));
    TEST_ASSERT_FALSE(second.skip());
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(CompactCborStatus::UnsupportedValue),
        static_cast<int>(second.status()));
}

void test_truncation_type_mismatch_and_output_limit_are_explicit() {
    const uint8_t truncated[] = {0x63, 'a'};
    CompactCborReader reader(truncated, sizeof(truncated));
    const char* text = nullptr;
    size_t size = 0;
    TEST_ASSERT_FALSE(reader.readText(text, size));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(CompactCborStatus::EndOfInput),
        static_cast<int>(reader.status()));

    uint8_t encoded[1];
    CompactCborWriter writer(encoded, sizeof(encoded));
    TEST_ASSERT_FALSE(writer.writeText("too long"));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(CompactCborStatus::OutputTooSmall),
        static_cast<int>(writer.status()));
}

void test_normative_vocabulary_numbers_remain_stable() {
    TEST_ASSERT_EQUAL_UINT8(0, static_cast<uint8_t>(HardwareDescriptorKey::SchemaVersion));
    TEST_ASSERT_EQUAL_UINT8(45, static_cast<uint8_t>(HardwareDescriptorKey::Values));
    TEST_ASSERT_EQUAL_UINT8(1, static_cast<uint8_t>(HardwareDescriptorPlatformCode::Esp32));
    TEST_ASSERT_EQUAL_UINT8(2, static_cast<uint8_t>(HardwareDescriptorDriverCode::Ads1115));
    TEST_ASSERT_EQUAL_UINT8(7, static_cast<uint8_t>(HardwareDescriptorCapabilityCode::SensorCurrent));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_writer_matches_deterministic_cbor_vector);
    RUN_TEST(test_reader_round_trips_bounded_views_and_double);
    RUN_TEST(test_reader_skips_unknown_nested_definite_values);
    RUN_TEST(test_noncanonical_and_indefinite_encodings_are_rejected);
    RUN_TEST(test_truncation_type_mismatch_and_output_limit_are_explicit);
    RUN_TEST(test_normative_vocabulary_numbers_remain_stable);
    return UNITY_END();
}
