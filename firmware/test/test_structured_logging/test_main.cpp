#include <Arduino.h>
#include <unity.h>

#include <cstring>
#include <string>
#include <vector>

#include "ILogEntrySink.h"
#include "ILogTimeProvider.h"
#include "LogLevel.h"
#include "RecentLogStore.h"
#include "StructuredLogger.h"

using namespace EnvNode;

HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t length) { return length; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}

namespace {

class TestTimeProvider : public ILogTimeProvider {
public:
    uint32_t monotonic = 0;
    bool valid = false;
    int64_t epoch = 0;

    uint32_t monotonicMs() const override { return monotonic; }
    bool wallClockEpochSeconds(int64_t& result) const override {
        if (!valid) return false;
        result = epoch;
        return true;
    }
};

class TestSink : public ILogEntrySink {
public:
    explicit TestSink(const RecentLogStore& store)
        : store_(store) {
    }

    void begin(unsigned long value) override { baud = value; }
    void write(const LogEntry& entry) override {
        storeCountWhenWritten.push_back(store_.count());
        entries.push_back(entry);
    }

    unsigned long baud = 0;
    std::vector<LogEntry> entries;
    std::vector<size_t> storeCountWhenWritten;

private:
    const RecentLogStore& store_;
};

struct Fixture {
    RecentLogStore store;
    TestTimeProvider time;
    TestSink sink;
    StructuredLogger logger;

    Fixture()
        : sink(store)
        , logger(store, time, sink) {
    }
};

LogEntry entryAt(const RecentLogStore& store, size_t index) {
    LogEntry entry;
    TEST_ASSERT_TRUE(store.copyEntry(index, entry));
    return entry;
}

void test_level_names_and_default_filtering() {
    TEST_ASSERT_EQUAL_STRING("DEBUG", logLevelDisplayName(LogLevel::Debug));
    TEST_ASSERT_EQUAL_STRING("INFO", logLevelDisplayName(LogLevel::Info));
    TEST_ASSERT_EQUAL_STRING("WARN", logLevelDisplayName(LogLevel::Warn));
    TEST_ASSERT_EQUAL_STRING("ERROR", logLevelDisplayName(LogLevel::Error));

    Fixture fixture;
    fixture.logger.debug("detail");
    fixture.logger.info("ready");
    fixture.logger.warn("degraded");
    fixture.logger.error("failed");
    TEST_ASSERT_EQUAL_UINT32(3, fixture.store.count());
    TEST_ASSERT_EQUAL_UINT32(3, fixture.sink.entries.size());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(LogLevel::Info),
        static_cast<int>(entryAt(fixture.store, 0).level));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(LogLevel::Warn),
        static_cast<int>(entryAt(fixture.store, 1).level));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(LogLevel::Error),
        static_cast<int>(entryAt(fixture.store, 2).level));
    TEST_ASSERT_EQUAL_UINT32(1, entryAt(fixture.store, 0).sequence);
    TEST_ASSERT_EQUAL_UINT32(3, entryAt(fixture.store, 2).sequence);
}

void test_runtime_minimum_level_accepts_debug_when_requested() {
    Fixture fixture;
    fixture.logger.setMinimumLevel(LogLevel::Debug);
    fixture.logger.debugf("value=%u", 17U);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.store.count());
    TEST_ASSERT_EQUAL_STRING("value=17", entryAt(fixture.store, 0).message);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(LogLevel::Debug),
        static_cast<int>(entryAt(fixture.store, 0).level));
}

void test_messages_are_owned_and_formatted_arguments_are_copied() {
    Fixture fixture;
    char stackMessage[] = "original";
    fixture.logger.info(stackMessage);
    stackMessage[0] = 'X';
    TEST_ASSERT_EQUAL_STRING("original", entryAt(fixture.store, 0).message);

    String temporary("temporary String");
    fixture.logger.info(temporary.c_str());
    temporary = "changed";
    TEST_ASSERT_EQUAL_STRING("temporary String", entryAt(fixture.store, 1).message);

    char argument[] = "argument";
    fixture.logger.infof("formatted %s", argument);
    argument[0] = 'X';
    TEST_ASSERT_EQUAL_STRING("formatted argument", entryAt(fixture.store, 2).message);
}

void test_message_boundaries_truncation_and_newline_normalization() {
    Fixture fixture;
    fixture.logger.info("");
    fixture.logger.info("line one\r\nline two\nline three\r");
    TEST_ASSERT_EQUAL_STRING("", entryAt(fixture.store, 0).message);
    TEST_ASSERT_EQUAL_STRING("line one line two line three",
        entryAt(fixture.store, 1).message);

    const std::string exact(LogMessageCapacity - 1, 'a');
    fixture.logger.info(exact.c_str());
    const LogEntry exactEntry = entryAt(fixture.store, 2);
    TEST_ASSERT_EQUAL_UINT32(LogMessageCapacity - 1, strlen(exactEntry.message));
    TEST_ASSERT_EQUAL_CHAR('a', exactEntry.message[LogMessageCapacity - 2]);
    TEST_ASSERT_EQUAL_CHAR('\0', exactEntry.message[LogMessageCapacity - 1]);

    const std::string oversized(LogMessageCapacity + 40, 'b');
    fixture.logger.info(oversized.c_str());
    const LogEntry truncated = entryAt(fixture.store, 3);
    TEST_ASSERT_EQUAL_UINT32(LogMessageCapacity - 1, strlen(truncated.message));
    TEST_ASSERT_EQUAL_STRING("...", truncated.message + LogMessageCapacity - 4);
    TEST_ASSERT_EQUAL_CHAR('\0', truncated.message[LogMessageCapacity - 1]);
}

void test_ring_buffer_orders_overwrites_wraps_and_clears() {
    RecentLogStore store;
    TEST_ASSERT_EQUAL_UINT32(0, store.count());
    LogEntry result;
    TEST_ASSERT_FALSE(store.copyEntry(0, result));

    for (size_t index = 0; index < RecentLogCapacity + 7; ++index) {
        LogEntry entry;
        entry.sequence = static_cast<uint32_t>(index + 1);
        store.append(entry);
    }
    TEST_ASSERT_EQUAL_UINT32(RecentLogCapacity, store.count());
    TEST_ASSERT_EQUAL_UINT32(8, entryAt(store, 0).sequence);
    TEST_ASSERT_EQUAL_UINT32(RecentLogCapacity + 7,
        entryAt(store, RecentLogCapacity - 1).sequence);
    TEST_ASSERT_FALSE(store.copyEntry(RecentLogCapacity, result));

    store.clear();
    TEST_ASSERT_EQUAL_UINT32(0, store.count());
    LogEntry replacement;
    replacement.sequence = 99;
    store.append(replacement);
    TEST_ASSERT_EQUAL_UINT32(99, entryAt(store, 0).sequence);
}

void test_timestamp_transition_does_not_modify_earlier_entry() {
    Fixture fixture;
    fixture.time.monotonic = 1234;
    fixture.logger.info("before sync");
    const LogEntry before = entryAt(fixture.store, 0);
    TEST_ASSERT_EQUAL_UINT32(1234, before.monotonicMs);
    TEST_ASSERT_FALSE(before.wallClockValid);
    TEST_ASSERT_EQUAL_INT64(0, before.epochSeconds);

    fixture.time.monotonic = 2345;
    fixture.time.valid = true;
    fixture.time.epoch = 1786986900LL;
    fixture.logger.info("after sync");
    const LogEntry after = entryAt(fixture.store, 1);
    TEST_ASSERT_EQUAL_UINT32(2345, after.monotonicMs);
    TEST_ASSERT_TRUE(after.wallClockValid);
    TEST_ASSERT_EQUAL_INT64(1786986900LL, after.epochSeconds);

    const LogEntry unchanged = entryAt(fixture.store, 0);
    TEST_ASSERT_FALSE(unchanged.wallClockValid);
    TEST_ASSERT_EQUAL_INT64(0, unchanged.epochSeconds);
}

void test_store_is_updated_before_sink_and_legacy_api_maps_to_info() {
    Fixture fixture;
    fixture.logger.begin(115200);
    fixture.logger.println("legacy line\n");
    fixture.logger.printf("legacy value=%u\n", 23U);
    TEST_ASSERT_EQUAL_UINT32(115200, fixture.sink.baud);
    TEST_ASSERT_EQUAL_UINT32(2, fixture.store.count());
    TEST_ASSERT_EQUAL_UINT32(2, fixture.sink.entries.size());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.sink.storeCountWhenWritten[0]);
    TEST_ASSERT_EQUAL_UINT32(2, fixture.sink.storeCountWhenWritten[1]);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(LogLevel::Info),
        static_cast<int>(entryAt(fixture.store, 0).level));
    TEST_ASSERT_EQUAL_STRING("legacy line", entryAt(fixture.store, 0).message);
    TEST_ASSERT_EQUAL_STRING("legacy value=23", entryAt(fixture.store, 1).message);
}

} // namespace

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_level_names_and_default_filtering);
    RUN_TEST(test_runtime_minimum_level_accepts_debug_when_requested);
    RUN_TEST(test_messages_are_owned_and_formatted_arguments_are_copied);
    RUN_TEST(test_message_boundaries_truncation_and_newline_normalization);
    RUN_TEST(test_ring_buffer_orders_overwrites_wraps_and_clears);
    RUN_TEST(test_timestamp_transition_does_not_modify_earlier_entry);
    RUN_TEST(test_store_is_updated_before_sink_and_legacy_api_maps_to_info);
    return UNITY_END();
}
