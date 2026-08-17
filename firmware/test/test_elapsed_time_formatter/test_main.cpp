#include <Arduino.h>
#include <unity.h>

#include <stdint.h>

#include "ElapsedTimeFormatter.h"

using namespace EnvNode;

HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t length) { return length; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}

namespace {

void assertFormatted(uint32_t elapsedMs, const char* expected) {
    TEST_ASSERT_EQUAL_STRING(expected, formatElapsedDuration(elapsedMs).c_str());
}

void test_elapsed_duration_uses_floor_based_compact_units() {
    assertFormatted(0, "0 s");
    assertFormatted(999, "0 s");
    assertFormatted(1000, "1 s");
    assertFormatted(59000, "59 s");
    assertFormatted(59999, "59 s");
    assertFormatted(60000, "1 min");
    assertFormatted(119999, "1 min");
    assertFormatted(120000, "2 min");
    assertFormatted(3599999, "59 min");
    assertFormatted(3600000, "1 h");
    assertFormatted(7199999, "1 h");
    assertFormatted(7200000, "2 h");
    assertFormatted(86399999, "23 h");
    assertFormatted(86400000, "1 d");
    assertFormatted(172800000, "2 d");
    assertFormatted(UINT32_MAX, "49 d");
}

void test_unsigned_monotonic_subtraction_remains_wrap_safe() {
    const uint32_t acceptedMs = UINT32_MAX - 10U;
    const uint32_t nowMs = 25U;
    const uint32_t ageMs = nowMs - acceptedMs;

    TEST_ASSERT_EQUAL_UINT32(36U, ageMs);
    assertFormatted(ageMs, "0 s");
}

} // namespace

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_elapsed_duration_uses_floor_based_compact_units);
    RUN_TEST(test_unsigned_monotonic_subtraction_remains_wrap_safe);
    return UNITY_END();
}
