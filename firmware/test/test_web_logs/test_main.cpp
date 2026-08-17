#include <Arduino.h>
#include <unity.h>

#include <cstdio>
#include <cstring>
#include <ctime>

#include "LogWebView.h"
#include "RecentLogStore.h"
#include "WebNavigation.h"

using namespace EnvNode;

HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t length) { return length; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}

namespace {

LogEntry makeEntry(
    uint32_t sequence,
    LogLevel level,
    const char* message,
    uint32_t monotonicMs = 0,
    bool wallClockValid = false,
    int64_t epochSeconds = 0) {
    LogEntry entry;
    entry.sequence = sequence;
    entry.level = level;
    entry.monotonicMs = monotonicMs;
    entry.wallClockValid = wallClockValid;
    entry.epochSeconds = epochSeconds;
    snprintf(entry.message, sizeof(entry.message), "%s", message);
    return entry;
}

void assertContains(const String& value, const char* expected) {
    TEST_ASSERT_NOT_NULL(strstr(value.c_str(), expected));
}

void testNavigationIncludesActiveLogsLink() {
    const String html = buildWebNavigationHtml("/logs");
    assertContains(html, "href='/logs' class='active'>Logs</a>");
}

void testEmptyStoreRendersNeutralStateAndCapacity() {
    RecentLogStore store;
    const String html = buildRecentLogHtml(store);
    assertContains(html, "Recent in-memory log entries: 0 / 64");
    assertContains(html, "No log entries available.");
    TEST_ASSERT_NULL(strstr(html.c_str(), "<table"));
}

void testEntriesRenderNewestFirstWithCanonicalSequenceAndTimes() {
    setenv("TZ", "UTC0", 1);
    tzset();
    RecentLogStore store;
    store.append(makeEntry(41, LogLevel::Info, "Boot", 3723004));
    store.append(makeEntry(42, LogLevel::Warn, "Clock ready", 0, true, 1786986900));
    const String html = buildRecentLogHtml(store);
    const char* newer = strstr(html.c_str(), "#42");
    const char* older = strstr(html.c_str(), "#41");
    TEST_ASSERT_NOT_NULL(newer);
    TEST_ASSERT_NOT_NULL(older);
    TEST_ASSERT_TRUE(newer < older);
    assertContains(html, "+01:02:03.004");
    assertContains(html, "2026-08-17 17:15:00");
}

void testLevelsAndMessageEscapingAreRendered() {
    RecentLogStore store;
    store.append(makeEntry(1, LogLevel::Debug, "debug"));
    store.append(makeEntry(2, LogLevel::Info, "info"));
    store.append(makeEntry(3, LogLevel::Warn, "warn"));
    store.append(makeEntry(4, LogLevel::Error, "<script>&\"' bad</script>"));
    const String html = buildRecentLogHtml(store);
    assertContains(html, "class='badge log-level debug'>DEBUG</span>");
    assertContains(html, "class='badge log-level info'>INFO</span>");
    assertContains(html, "class='badge warn log-level'>WARN</span>");
    assertContains(html, "class='badge bad log-level'>ERROR</span>");
    assertContains(html, "&lt;script&gt;&amp;&quot;&#39; bad&lt;/script&gt;");
    TEST_ASSERT_NULL(strstr(html.c_str(), "<script>"));
}

void testFullStoreShowsRetainedEntriesAndWrappingMarkup() {
    RecentLogStore store;
    char message[LogMessageCapacity];
    memset(message, 'x', sizeof(message) - 1);
    message[sizeof(message) - 1] = '\0';
    for (uint32_t sequence = 1; sequence <= 70; ++sequence) {
        store.append(makeEntry(sequence, LogLevel::Info, message));
    }
    const String html = buildRecentLogHtml(store);
    assertContains(html, "Recent in-memory log entries: 64 / 64");
    assertContains(html, "table-scroll log-table-wrap");
    assertContains(html, "class='log-table'");
    TEST_ASSERT_TRUE(strstr(html.c_str(), ">#70</td>") < strstr(html.c_str(), ">#7</td>"));
    TEST_ASSERT_NULL(strstr(html.c_str(), ">#6</td>"));
    TEST_ASSERT_TRUE(html.length() < 30000);
}

} // namespace

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(testNavigationIncludesActiveLogsLink);
    RUN_TEST(testEmptyStoreRendersNeutralStateAndCapacity);
    RUN_TEST(testEntriesRenderNewestFirstWithCanonicalSequenceAndTimes);
    RUN_TEST(testLevelsAndMessageEscapingAreRendered);
    RUN_TEST(testFullStoreShowsRetainedEntriesAndWrappingMarkup);
    return UNITY_END();
}
