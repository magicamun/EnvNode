#include <unity.h>
#include "TimePropertyReader.h"
#include "SystemPropertyReader.h"
#include "ConfigurationService.h"
#include "PropertyTextFormatter.h"
using namespace EnvNode;
HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t length) { return length; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}
void setUp() { Preferences().clear(); }
void tearDown() {}
struct Time : ITimeService {
    bool synced = true, localValid = true;
    tm local{};
    Time() { local.tm_year=126; local.tm_mon=9; local.tm_mday=7; local.tm_hour=14; local.tm_min=4; local.tm_sec=2; }
    void begin() override {}
    void loop() override {}
    bool synchronized() const override { return synced; }
    time_t now() const override { return 0; }
    bool localCivilTime(tm& target) const override { target = local; return localValid; }
    String iso8601Utc() const override { return ""; }
    String iso8601Local() const override { return ""; }
    String iso8601Local(time_t) const override { return ""; }
    uint32_t epoch() const override { return 0; }
};
void test_locale_text_sources_and_owned_snapshots() {
    ConfigurationService config; config.loadConfiguration();
    LocaleFormatter locale(config); Time time; TimePropertyReader reader(time, locale);
    PropertyReference date(PropertyComponentKind::System,1,"date"), clock(PropertyComponentKind::System,1,"time"), both(PropertyComponentKind::System,1,"datetime");
    TEST_ASSERT_EQUAL_STRING("07.10.2026", formatPropertyText(reader,date,"%s").text);
    TEST_ASSERT_EQUAL_STRING("14:04:02", formatPropertyText(reader,clock,"%s").text);
    TEST_ASSERT_EQUAL_STRING("07.10.2026 14:04:02", formatPropertyText(reader,both,"%s").text);
    config.setLocale(Locale::EnglishUnitedStates);
    TEST_ASSERT_EQUAL_STRING("10/07/2026 02:04:02 PM", formatPropertyText(reader,both,"%s").text);
    config.setLocale(Locale::EnglishUnitedKingdom);
    TEST_ASSERT_EQUAL_STRING("07/10/2026 14:04:02", formatPropertyText(reader,both,"%s").text);
    PropertySnapshot first, second;
    reader.read(clock,first); time.local.tm_hour=15; reader.read(clock,second);
    const char* text = nullptr;
    TEST_ASSERT_TRUE(first.value.tryGetText(text)); TEST_ASSERT_EQUAL_STRING("14:04:02",text);
    TEST_ASSERT_TRUE(second.value.tryGetText(text)); TEST_ASSERT_EQUAL_STRING("15:04:02",text);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyFormatStatus::TypeMismatch), static_cast<int>(formatPropertyText(reader,clock,"%u").status));
}
void test_time_is_unavailable_until_synchronized_and_local_time_valid() {
    ConfigurationService config; config.loadConfiguration(); LocaleFormatter locale(config);
    Time time; TimePropertyReader reader(time,locale);
    PropertyReference source(PropertyComponentKind::System,1,"date");
    PropertyDescription description; TEST_ASSERT_TRUE(reader.describe(source,description));
    time.synced=false;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyFormatStatus::NoValue),static_cast<int>(formatPropertyText(reader,source,"%s").status));
    time.synced=true; time.localValid=false;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyFormatStatus::NoValue),static_cast<int>(formatPropertyText(reader,source,"%s").status));
    PropertySnapshot snapshot;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyReadResult::UnknownReference),static_cast<int>(reader.read({PropertyComponentKind::System,2,"date"},snapshot)));
    TEST_ASSERT_FALSE(snapshot.valid);
}
void test_build_sources_work_without_time_sync() {
    ConfigurationService config; config.loadConfiguration(); LocaleFormatter locale(config);
    Time time; time.synced = false;
    SystemPropertyReader reader(time, locale, "0.5.0", "312", "abcdef0", "0.5.0+312.gabcdef0");
    TEST_ASSERT_EQUAL_STRING("0.5.0", formatPropertyText(reader,{PropertyComponentKind::System,1,"version"},"%s").text);
    TEST_ASSERT_EQUAL_STRING("312", formatPropertyText(reader,{PropertyComponentKind::System,1,"build"},"%s").text);
    TEST_ASSERT_EQUAL_STRING("abcdef0", formatPropertyText(reader,{PropertyComponentKind::System,1,"git_commit"},"%s").text);
    TEST_ASSERT_EQUAL_STRING("0.5.0+312.gabcdef0", formatPropertyText(reader,{PropertyComponentKind::System,1,"build_identity"},"%s").text);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyFormatStatus::NoValue),static_cast<int>(formatPropertyText(reader,{PropertyComponentKind::System,1,"date"},"%s").status));
    PropertyDescription description;
    TEST_ASSERT_FALSE(reader.describe({PropertyComponentKind::System,2,"version"},description));
    TEST_ASSERT_FALSE(reader.describe({PropertyComponentKind::System,1,"missing"},description));
}
int main(int,char**) { UNITY_BEGIN(); RUN_TEST(test_build_sources_work_without_time_sync); RUN_TEST(test_locale_text_sources_and_owned_snapshots); RUN_TEST(test_time_is_unavailable_until_synchronized_and_local_time_valid); return UNITY_END(); }
