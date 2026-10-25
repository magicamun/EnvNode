// EnvNode: see repository LICENSE.
#include <unity.h>
#include "TimeService.h"
#include "LocaleFormatter.h"
#include "esp_sntp.h"
#include <cstdlib>
#include <string>
using namespace EnvNode;
HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*,size_t n){return n;}
void pinMode(unsigned char,int){}
void digitalWrite(unsigned char,int){}
struct Logger:ILogger {void begin(unsigned long) override{} void println(const char*) override{} void printf(const char*,...) override{}};
struct WiFi:IWiFiService {
    bool online=false;
    void begin() override{} void loop() override{}
    bool connected() const override{return online;} bool inSetupAccessPointMode() const override{return false;}
    String ipAddress() const override{return "";} String hostname() const override{return "node";}
    int32_t rssi() const override{return -42;}
};
struct Settings:ITimeConfiguration,ILocaleConfiguration {
    TimeConfiguration time;
    Locale locale=Locale::GermanGermany;
    const TimeConfiguration& getTimeConfiguration() const override{return time;}
    Locale getLocale() const override{return locale;}
};
std::string previousTimezone;bool timezoneWasSet=false;
void setUp(){nativeSntp()=NativeSntpState();const char* tz=getenv("TZ");timezoneWasSet=tz!=nullptr;previousTimezone=tz?tz:"";}
void tearDown(){if(timezoneWasSet)setenv("TZ",previousTimezone.c_str(),1);else unsetenv("TZ");tzset();}
void test_no_sntp_before_wifi_and_defaults_are_preserved(){
    Logger logger;WiFi wifi;Settings settings;TimeService service(logger,settings,wifi);
    service.begin();service.loop();TEST_ASSERT_EQUAL(0,nativeSntp().starts);TEST_ASSERT_FALSE(service.synchronized());
    wifi.online=true;service.loop();
    TEST_ASSERT_EQUAL(1,nativeSntp().starts);TEST_ASSERT_FALSE(service.synchronized());
    TEST_ASSERT_EQUAL_STRING("pool.ntp.org",nativeSntp().servers[0].c_str());
    TEST_ASSERT_EQUAL_STRING("time.cloudflare.com",nativeSntp().servers[1].c_str());
    TEST_ASSERT_TRUE(nativeSntp().servers[2].empty());
    TEST_ASSERT_EQUAL_STRING("CET-1CEST,M3.5.0/2,M10.5.0/3",getenv("TZ"));
    service.loop();TEST_ASSERT_TRUE(service.synchronized());service.loop();TEST_ASSERT_EQUAL(1,nativeSntp().starts);
}
void test_custom_settings_restart_existing_sntp_in_order(){
    Logger logger;WiFi wifi;wifi.online=true;Settings settings;
    settings.time={"UTC0","ntp-one.local","ntp-two.local"};nativeSntp().enabled=true;
    TimeService service(logger,settings,wifi);service.begin();service.loop();
    TEST_ASSERT_EQUAL(1,nativeSntp().stops);TEST_ASSERT_EQUAL_STRING("stop",nativeSntp().events.front().c_str());
    TEST_ASSERT_EQUAL_STRING("start",nativeSntp().events.back().c_str());
    TEST_ASSERT_EQUAL_STRING("ntp-one.local",nativeSntp().servers[0].c_str());
    TEST_ASSERT_EQUAL_STRING("ntp-two.local",nativeSntp().servers[1].c_str());
    TEST_ASSERT_EQUAL_STRING("UTC0",getenv("TZ"));
    service.begin();TEST_ASSERT_FALSE(service.synchronized());service.loop();TEST_ASSERT_EQUAL(2,nativeSntp().starts);
}
void test_iso_local_offsets_follow_timezone_for_fixed_timestamp(){
    Logger logger;WiFi wifi;Settings settings;TimeService service(logger,settings,wifi);
    setenv("TZ","UTC0",1);tzset();
    TEST_ASSERT_EQUAL_STRING("1970-01-01T00:00:00+00:00",service.iso8601Local(0).c_str());
    setenv("TZ","CET-1",1);tzset();
    TEST_ASSERT_EQUAL_STRING("1970-01-01T01:00:00+01:00",service.iso8601Local(0).c_str());
    tm local;TEST_ASSERT_TRUE(service.localCivilTime(local));
    TEST_ASSERT_NOT_NULL(strchr(service.iso8601Utc().c_str(),'Z'));
}
void test_locale_formatting_and_live_locale_changes(){
    Settings settings;LocaleFormatter formatter(settings);tm time={};time.tm_year=126;time.tm_mon=9;time.tm_mday=25;time.tm_hour=13;time.tm_min=4;time.tm_sec=5;
    TEST_ASSERT_EQUAL_STRING("25.10.2026 13:04:05",formatter.formatDateTime(time).c_str());
    TEST_ASSERT_EQUAL_STRING("-1.234.567,50",formatter.formatNumber(-1234567.5,2).c_str());
    settings.locale=Locale::EnglishUnitedKingdom;
    TEST_ASSERT_EQUAL_STRING("25/10/2026 13:04:05",formatter.formatDateTime(time).c_str());
    TEST_ASSERT_EQUAL_STRING("1,234.50",formatter.formatNumber(1234.5,2).c_str());
    settings.locale=Locale::EnglishUnitedStates;
    TEST_ASSERT_EQUAL_STRING("10/25/2026 01:04:05 PM",formatter.formatDateTime(time).c_str());
    time.tm_hour=0;TEST_ASSERT_EQUAL_STRING("12:04:05 AM",formatter.formatTime(time).c_str());
    TEST_ASSERT_EQUAL_STRING("1.250000",formatter.formatNumber(1.25,9).c_str());
}
int main(){UNITY_BEGIN();RUN_TEST(test_no_sntp_before_wifi_and_defaults_are_preserved);RUN_TEST(test_custom_settings_restart_existing_sntp_in_order);RUN_TEST(test_iso_local_offsets_follow_timezone_for_fixed_timestamp);RUN_TEST(test_locale_formatting_and_live_locale_changes);return UNITY_END();}
