// EnvNode: see repository LICENSE.
#include <unity.h>
#include "CommonSettingsWebRoutes.h"
#include <string>
#include <vector>
#include <cstring>
using namespace EnvNode;
HardwareSerial Serial;
void HardwareSerial::begin(unsigned long){}
void HardwareSerial::println(const char*){}
size_t HardwareSerial::write(const uint8_t*,size_t n){return n;}
void pinMode(unsigned char,int){}
void digitalWrite(unsigned char,int){}
void setUp(){} void tearDown(){}
struct Logger:ILogger{void begin(unsigned long)override{}void println(const char*)override{}void printf(const char*,...)override{}};
struct WiFi:IWiFiService{
    bool online=true,setup=false;
    void begin()override{}void loop()override{}bool connected()const override{return online;}
    bool inSetupAccessPointMode()const override{return setup;}String ipAddress()const override{return "";}
    String hostname()const override{return "node";}int32_t rssi()const override{return -42;}
};
struct Settings:IBaseSettings{
    TimeConfiguration time={"UTC0","one.local","two.local"};DeviceConfiguration device={"Node <test>"};Locale locale=Locale::EnglishUnitedKingdom;
    std::vector<std::string> writes;std::string fail;
    Locale getLocale()const override{return locale;}
    const TimeConfiguration& getTimeConfiguration()const override{return time;}
    const DeviceConfiguration& getDeviceConfiguration()const override{return device;}
    bool accept(const char* key){writes.push_back(key);return fail!=key;}
    bool setDeviceName(const String& name)override{if(!accept("device"))return false;device.name=name;return true;}
    bool setLocale(Locale value)override{if(!accept("locale"))return false;locale=value;return true;}
    bool setTimezone(const String& value)override{if(!accept("timezone"))return false;time.timezone=value;return true;}
    bool setNtpServer1(const String& value)override{if(!accept("ntp1"))return false;time.ntpServer1=value;return true;}
    bool setNtpServer2(const String& value)override{if(!accept("ntp2"))return false;time.ntpServer2=value;return true;}
};
struct Pages:IWebPagePresenter,IWebPageHeaderPresenter{
    Settings& settings;String content;int status=0;bool headerOk=true;mutable int headers=0;
    explicit Pages(Settings& value):settings(value){}
    Locale webLocale()const override{return settings.locale;}
    void sendPage(const char*,const char*,const String& c,int s,bool)override{content=c;status=s;}
    String renderPageHeader(const char*,const char*,bool)const override{++headers;return headerOk?"<html><body><div><main>":"";}
};
struct Time:ITimeService{
    bool synced=true;
    void begin()override{}void loop()override{}bool synchronized()const override{return synced;}
    time_t now()const override{return 0;}bool localCivilTime(tm& time)const override{time={};time.tm_year=126;time.tm_mon=9;time.tm_mday=25;return true;}
    String iso8601Utc()const override{return "";}String iso8601Local()const override{return "";}
    String iso8601Local(time_t)const override{return "";}uint32_t epoch()const override{return 0;}
};
struct Fixture{
    Logger logger;WiFi wifi;Settings settings;Pages pages{settings};Time time;LocaleFormatter formatter{settings};WebHost host{logger,wifi};
    int timeCalls=0,deviceCalls=0;bool success=false,localeChanged=false,timeChanged=false;
    CommonSettingsWebRoutes routes{host,pages,pages,settings,time,formatter,[](){return String("<section>EnvNode extras</section>");},
        [this](bool ok,bool locale,bool time){++timeCalls;success=ok;localeChanged=locale;timeChanged=time;},
        [this](bool ok){++deviceCalls;success=ok;}};
    Fixture(){host.registerRoutes(routes);}
    void form(const char* locale="en-GB"){host.server().arguments={{"locale",locale},{"timezone","UTC0"},{"ntpServer1","one.local"},{"ntpServer2","two.local"}};}
};
void test_streamed_time_page_keeps_timezone_catalog_custom_value_and_escaped_ntp(){
    Fixture f;f.settings.time.timezone="CUSTOM'<tz>";f.settings.time.ntpServer1="ntp'<one>";
    f.host.server().request("/time",HTTP_GET);const auto& server=f.host.server();
    TEST_ASSERT_EQUAL(4,server.routes.size());TEST_ASSERT_EQUAL(200,server.responseStatus);
    TEST_ASSERT_EQUAL(CONTENT_LENGTH_UNKNOWN,server.declaredLength);
    TEST_ASSERT_NOT_NULL(strstr(server.responseBody.c_str(),"CUSTOM&#39;&lt;tz&gt;"));
    TEST_ASSERT_NOT_NULL(strstr(server.responseBody.c_str(),"ntp&#39;&lt;one&gt;"));
    TEST_ASSERT_NOT_NULL(strstr(server.responseBody.c_str(),"name='locale'"));
    TEST_ASSERT_NOT_NULL(strstr(server.responseBody.c_str(),"name='timezone'"));
    TEST_ASSERT_NOT_NULL(strstr(server.responseBody.c_str(),"</main></div></body></html>"));
    TEST_ASSERT_GREATER_THAN(5,server.chunks.size());
    for(const auto& chunk:server.chunks)TEST_ASSERT_LESS_THAN(3000,chunk.size());
}
void test_time_page_guards_offline_and_failed_header_but_accepts_setup_ap(){
    Fixture f;f.wifi.online=false;f.host.server().request("/time",HTTP_GET);
    TEST_ASSERT_EQUAL(503,f.host.server().responseStatus);TEST_ASSERT_EQUAL(0,f.pages.headers);
    f.wifi.setup=true;f.pages.headerOk=false;f.host.server().request("/time",HTTP_GET);
    TEST_ASSERT_EQUAL(503,f.host.server().responseStatus);
    f.pages.headerOk=true;f.host.server().request("/time",HTTP_GET);TEST_ASSERT_EQUAL(200,f.host.server().responseStatus);
}
void test_time_save_reports_unchanged_locale_and_time_without_extra_application_dependencies(){
    Fixture f;f.form();f.host.server().request("/time/save",HTTP_POST);
    TEST_ASSERT_TRUE(f.success);TEST_ASSERT_EQUAL(1,f.timeCalls);TEST_ASSERT_FALSE(f.localeChanged);TEST_ASSERT_FALSE(f.timeChanged);
    TEST_ASSERT_EQUAL(4,f.settings.writes.size());TEST_ASSERT_EQUAL_STRING("locale",f.settings.writes[0].c_str());
    TEST_ASSERT_EQUAL_STRING("timezone",f.settings.writes[1].c_str());TEST_ASSERT_EQUAL_STRING("ntp1",f.settings.writes[2].c_str());TEST_ASSERT_EQUAL_STRING("ntp2",f.settings.writes[3].c_str());
}
void test_time_save_detects_changes_and_preserves_short_circuit_failure(){
    Fixture f;f.form("de-DE");f.host.server().arguments["timezone"]="CET-1";f.settings.fail="ntp1";
    f.host.server().request("/time/save",HTTP_POST);
    TEST_ASSERT_FALSE(f.success);TEST_ASSERT_TRUE(f.localeChanged);TEST_ASSERT_TRUE(f.timeChanged);
    TEST_ASSERT_EQUAL(3,f.settings.writes.size());TEST_ASSERT_EQUAL_STRING("CET-1",f.settings.time.timezone.c_str());
}
void test_invalid_locale_does_not_write_any_settings(){
    Fixture f;f.form("invalid");f.host.server().request("/time/save",HTTP_POST);
    TEST_ASSERT_FALSE(f.success);TEST_ASSERT_EQUAL(0,f.settings.writes.size());TEST_ASSERT_EQUAL(1,f.timeCalls);
}
void test_device_page_preserves_escaped_name_and_application_extras(){
    Fixture f;f.host.server().request("/device",HTTP_GET);
    TEST_ASSERT_EQUAL(200,f.pages.status);TEST_ASSERT_NOT_NULL(strstr(f.pages.content.c_str(),"Node &lt;test&gt;"));
    TEST_ASSERT_NOT_NULL(strstr(f.pages.content.c_str(),"EnvNode extras"));
    TEST_ASSERT_NOT_NULL(strstr(f.pages.content.c_str(),"action='/device/save'"));
}
void test_device_save_delegates_outcome_to_application(){
    Fixture f;f.host.server().arguments["deviceName"]="New node";f.host.server().request("/device/save",HTTP_POST);
    TEST_ASSERT_TRUE(f.success);TEST_ASSERT_EQUAL(1,f.deviceCalls);TEST_ASSERT_EQUAL_STRING("New node",f.settings.device.name.c_str());
    f.settings.fail="device";f.host.server().request("/device/save",HTTP_POST);TEST_ASSERT_FALSE(f.success);TEST_ASSERT_EQUAL(2,f.deviceCalls);
}
int main(){UNITY_BEGIN();RUN_TEST(test_streamed_time_page_keeps_timezone_catalog_custom_value_and_escaped_ntp);RUN_TEST(test_time_page_guards_offline_and_failed_header_but_accepts_setup_ap);RUN_TEST(test_time_save_reports_unchanged_locale_and_time_without_extra_application_dependencies);RUN_TEST(test_time_save_detects_changes_and_preserves_short_circuit_failure);RUN_TEST(test_invalid_locale_does_not_write_any_settings);RUN_TEST(test_device_page_preserves_escaped_name_and_application_extras);RUN_TEST(test_device_save_delegates_outcome_to_application);return UNITY_END();}
