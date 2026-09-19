#include <cstring>

#include <Arduino.h>
#include <unity.h>

#include "BoardIdentityCodec.h"

using namespace EnvNode;

HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
size_t HardwareSerial::write(const uint8_t*, size_t length) { return length; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}

namespace {

const uint8_t ExampleRecord[BoardIdentityCodec::EncodedSize] = {
    0x45, 0x4E, 0x49, 0x44, 0x01, 0x20, 0x00, 0x00,
    0x00, 0x03, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x8E, 0xB2, 0x04, 0x3C,
};

void writeCrc(uint8_t* record) {
    const uint32_t crc = BoardIdentityCodec::calculateCrc32(record, 28);
    record[28] = static_cast<uint8_t>(crc);
    record[29] = static_cast<uint8_t>(crc >> 8);
    record[30] = static_cast<uint8_t>(crc >> 16);
    record[31] = static_cast<uint8_t>(crc >> 24);
}

} // namespace

void test_crc_matches_standard_check_value() {
    const uint8_t input[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    TEST_ASSERT_EQUAL_HEX32(
        0xCBF43926,
        BoardIdentityCodec::calculateCrc32(input, sizeof(input)));
}

void test_decode_documented_example() {
    BoardIdentity identity = {};
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardIdentityStatus::Valid),
        static_cast<int>(BoardIdentityCodec::decode(
            ExampleRecord, sizeof(ExampleRecord), identity)));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardProfileId::EnvNodeMainboard),
        static_cast<int>(identity.profileId));
    TEST_ASSERT_EQUAL_UINT8(0, identity.revision.major);
    TEST_ASSERT_EQUAL_UINT8(3, identity.revision.minor);
    TEST_ASSERT_EQUAL_UINT32(12, identity.serialNumber);
}

void test_encode_matches_documented_example() {
    const BoardIdentity identity = {
        BoardProfileId::EnvNodeMainboard,
        {0, 3},
        12,
    };
    uint8_t encoded[BoardIdentityCodec::EncodedSize];
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardIdentityStatus::Valid),
        static_cast<int>(BoardIdentityCodec::encode(identity, encoded, sizeof(encoded))));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(ExampleRecord, encoded, sizeof(encoded));
}

void test_blank_records_are_not_provisioned() {
    uint8_t erased[BoardIdentityCodec::EncodedSize];
    uint8_t zeroed[BoardIdentityCodec::EncodedSize];
    std::memset(erased, 0xFF, sizeof(erased));
    std::memset(zeroed, 0x00, sizeof(zeroed));
    BoardIdentity identity = {};
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardIdentityStatus::NotProvisioned),
        static_cast<int>(BoardIdentityCodec::decode(erased, sizeof(erased), identity)));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardIdentityStatus::NotProvisioned),
        static_cast<int>(BoardIdentityCodec::decode(zeroed, sizeof(zeroed), identity)));
}

void test_validation_order_distinguishes_format_length_and_crc() {
    uint8_t record[BoardIdentityCodec::EncodedSize];
    BoardIdentity identity = {};

    std::memcpy(record, ExampleRecord, sizeof(record));
    record[4] = 2;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardIdentityStatus::UnsupportedFormat),
        static_cast<int>(BoardIdentityCodec::decode(record, sizeof(record), identity)));

    std::memcpy(record, ExampleRecord, sizeof(record));
    record[5] = 31;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardIdentityStatus::InvalidLength),
        static_cast<int>(BoardIdentityCodec::decode(record, sizeof(record), identity)));

    std::memcpy(record, ExampleRecord, sizeof(record));
    record[10] ^= 0x01;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardIdentityStatus::InvalidCRC),
        static_cast<int>(BoardIdentityCodec::decode(record, sizeof(record), identity)));
}

void test_unknown_profile_does_not_fall_back() {
    uint8_t record[BoardIdentityCodec::EncodedSize];
    std::memcpy(record, ExampleRecord, sizeof(record));
    record[6] = 1;
    writeCrc(record);
    BoardIdentity identity = {};
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardIdentityStatus::UnknownBoardProfile),
        static_cast<int>(BoardIdentityCodec::decode(record, sizeof(record), identity)));
}

void test_revision_and_unassigned_serial_are_distinct() {
    uint8_t record[BoardIdentityCodec::EncodedSize];
    BoardIdentity identity = {};

    std::memcpy(record, ExampleRecord, sizeof(record));
    record[8] = 0xFF;
    record[9] = 0xFF;
    writeCrc(record);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardIdentityStatus::InvalidRevision),
        static_cast<int>(BoardIdentityCodec::decode(record, sizeof(record), identity)));

    std::memcpy(record, ExampleRecord, sizeof(record));
    record[9] = 1;
    writeCrc(record);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardIdentityStatus::UnsupportedRevision),
        static_cast<int>(BoardIdentityCodec::decode(record, sizeof(record), identity)));

    std::memcpy(record, ExampleRecord, sizeof(record));
    std::memset(record + 10, 0, 4);
    writeCrc(record);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardIdentityStatus::UnassignedSerial),
        static_cast<int>(BoardIdentityCodec::decode(record, sizeof(record), identity)));
}

void test_non_zero_reserved_bytes_are_accepted_when_crc_is_valid() {
    uint8_t record[BoardIdentityCodec::EncodedSize];
    std::memcpy(record, ExampleRecord, sizeof(record));
    record[14] = 0xA5;
    record[27] = 0x5A;
    writeCrc(record);
    BoardIdentity identity = {};
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardIdentityStatus::Valid),
        static_cast<int>(BoardIdentityCodec::decode(record, sizeof(record), identity)));
}

void test_current_mainboard_revision_is_supported() {
    const BoardIdentity identity = {
        BoardProfileId::EnvNodeMainboard,
        {0, 3},
        12,
    };
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardIdentityStatus::Valid),
        static_cast<int>(validateBoardIdentity(identity)));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_crc_matches_standard_check_value);
    RUN_TEST(test_decode_documented_example);
    RUN_TEST(test_encode_matches_documented_example);
    RUN_TEST(test_blank_records_are_not_provisioned);
    RUN_TEST(test_validation_order_distinguishes_format_length_and_crc);
    RUN_TEST(test_unknown_profile_does_not_fall_back);
    RUN_TEST(test_revision_and_unassigned_serial_are_distinct);
    RUN_TEST(test_non_zero_reserved_bytes_are_accepted_when_crc_is_valid);
    RUN_TEST(test_current_mainboard_revision_is_supported);
    return UNITY_END();
}
