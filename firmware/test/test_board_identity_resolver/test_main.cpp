#include <cstring>

#include <Arduino.h>
#include <unity.h>

#include "BoardIdentityResolver.h"

using namespace EnvNode;

HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
size_t HardwareSerial::write(const uint8_t*, size_t length) { return length; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}

namespace {

class MemoryStorage : public IBoardIdentityStorage {
public:
    MemoryStorage() {
        std::memset(bytes, 0xFF, sizeof(bytes));
    }

    bool read(uint8_t address, uint8_t* data, size_t size) override {
        ++reads;
        if (unavailable) return false;
        std::memcpy(data, bytes + address, size);
        return true;
    }

    bool write(uint8_t address, const uint8_t* data, size_t size) override {
        std::memcpy(bytes + address, data, size);
        return true;
    }

    void setIdentity(const BoardIdentity& identity) {
        BoardIdentityCodec::encode(identity, bytes, BoardIdentityCodec::EncodedSize);
    }

    uint8_t bytes[BoardIdentityCodec::EncodedSize];
    size_t reads = 0;
    bool unavailable = false;
};

BoardIdentityResolution resolve(MemoryStorage& storage) {
    BoardIdentityStore store(storage);
    BoardIdentityResolver resolver(store, BoardProfileId::EnvNodeMainboard);
    return resolver.resolve();
}

} // namespace

void test_valid_eeprom_identity_wins() {
    MemoryStorage storage;
    storage.setIdentity({BoardProfileId::EnvNodeMainboard, {0, 2}, 12});
    const BoardIdentityResolution result = resolve(storage);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardIdentitySource::EEPROM),
        static_cast<int>(result.source));
    TEST_ASSERT_TRUE(result.normalRuntimeAllowed);
    TEST_ASSERT_EQUAL_UINT32(12, result.identity.serialNumber);
}

void test_unassigned_serial_remains_usable_eeprom_identity() {
    MemoryStorage storage;
    storage.setIdentity({BoardProfileId::EnvNodeMainboard, {0, 2}, 0});
    const BoardIdentityResolution result = resolve(storage);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardIdentityStatus::UnassignedSerial),
        static_cast<int>(result.recordStatus));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardIdentitySource::EEPROM),
        static_cast<int>(result.source));
    TEST_ASSERT_TRUE(result.normalRuntimeAllowed);
}

void test_blank_unavailable_and_corrupt_records_use_visible_fallback() {
    MemoryStorage blank;
    BoardIdentityResolution result = resolve(blank);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardIdentitySource::BuildFallback),
        static_cast<int>(result.source));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardIdentityStatus::NotProvisioned),
        static_cast<int>(result.recordStatus));
    TEST_ASSERT_TRUE(result.normalRuntimeAllowed);

    MemoryStorage unavailable;
    unavailable.unavailable = true;
    result = resolve(unavailable);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardIdentitySource::BuildFallback),
        static_cast<int>(result.source));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardIdentityStatus::StorageUnavailable),
        static_cast<int>(result.recordStatus));

    MemoryStorage corrupt;
    corrupt.setIdentity({BoardProfileId::EnvNodeMainboard, {0, 2}, 12});
    corrupt.bytes[10] ^= 1;
    result = resolve(corrupt);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardIdentitySource::BuildFallback),
        static_cast<int>(result.source));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardIdentityStatus::InvalidCRC),
        static_cast<int>(result.recordStatus));
}

void test_unknown_profile_and_unsupported_revision_never_fall_back() {
    MemoryStorage unknown;
    unknown.setIdentity({BoardProfileId::EnvNodeMainboard, {0, 2}, 12});
    unknown.bytes[6] = 1;
    const uint32_t unknownCrc = BoardIdentityCodec::calculateCrc32(unknown.bytes, 28);
    for (size_t index = 0; index < 4; ++index) {
        unknown.bytes[28 + index] = static_cast<uint8_t>(unknownCrc >> (index * 8));
    }
    BoardIdentityResolution result = resolve(unknown);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardIdentitySource::UnsupportedIdentity),
        static_cast<int>(result.source));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardIdentityStatus::UnknownBoardProfile),
        static_cast<int>(result.recordStatus));
    TEST_ASSERT_FALSE(result.normalRuntimeAllowed);

    MemoryStorage unsupported;
    unsupported.setIdentity({BoardProfileId::EnvNodeMainboard, {0, 2}, 12});
    unsupported.bytes[9] = 1;
    const uint32_t revisionCrc = BoardIdentityCodec::calculateCrc32(unsupported.bytes, 28);
    for (size_t index = 0; index < 4; ++index) {
        unsupported.bytes[28 + index] = static_cast<uint8_t>(revisionCrc >> (index * 8));
    }
    result = resolve(unsupported);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardIdentityStatus::UnsupportedRevision),
        static_cast<int>(result.recordStatus));
    TEST_ASSERT_FALSE(result.normalRuntimeAllowed);
}

void test_resolution_is_read_and_frozen_once() {
    MemoryStorage storage;
    BoardIdentityStore store(storage);
    BoardIdentityResolver resolver(store, BoardProfileId::EnvNodeMainboard);
    const BoardIdentityResolution& first = resolver.resolve();
    storage.setIdentity({BoardProfileId::EnvNodeMainboard, {0, 2}, 12});
    const BoardIdentityResolution& second = resolver.resolve();
    TEST_ASSERT_EQUAL_UINT32(1, storage.reads);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardIdentitySource::BuildFallback),
        static_cast<int>(first.source));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(first.source), static_cast<int>(second.source));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_valid_eeprom_identity_wins);
    RUN_TEST(test_unassigned_serial_remains_usable_eeprom_identity);
    RUN_TEST(test_blank_unavailable_and_corrupt_records_use_visible_fallback);
    RUN_TEST(test_unknown_profile_and_unsupported_revision_never_fall_back);
    RUN_TEST(test_resolution_is_read_and_frozen_once);
    return UNITY_END();
}
