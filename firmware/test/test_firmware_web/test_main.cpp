// EnvNode: see repository LICENSE.
#include <unity.h>
#include "FirmwareWebRoutes.h"
#include "Update.h"
#include <cstring>
using namespace EnvNode;
UpdateClass Update;
HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t n) { return n; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}
void setUp() { Update = UpdateClass(); }
void tearDown() {}
struct Logger : ILogger {
 void begin(unsigned long) override {} void println(const char*) override {} void printf(const char*, ...) override {}
};
struct Pages : IWebPagePresenter {
 int status=0; String content;
 Locale webLocale() const override { return Locale::EnglishUnitedKingdom; }
 void sendPage(const char*, const char*, const String& c, int s, bool) override { status=s;content=c; }
};
struct Restart : IDeviceRestart {
 bool pending=false, responseBeforeRestart=false; int executions=0; Pages& pages; WebServer& server;
 Restart(Pages& p, WebServer& s):pages(p),server(s){}
 void requestDeviceRestart() override {pending=true;}
 bool deviceRestartPending() const override {return pending;}
 void performDeviceRestart() override {++executions;responseBeforeRestart=pages.status==200 && server.clientState.flushes==1;}
};
struct Fixture {
 WebServer server{80}; Logger logger; Pages pages; Restart restart{pages,server}; OTAService ota{logger,restart};
 bool resetOk=true; int resets=0;
 FirmwareWebRoutes routes{server,pages,ota,restart,{"0.7.0","identity","1","abc","branch<test>","now","clean",false},
  [this](){++resets;return resetOk;}, [](double value,uint8_t digits){return String(static_cast<float>(value),digits);}};
 Fixture(){routes.registerRoutes(server);}
 void event(HTTPUploadStatus status){server.uploadState.status=status;server.uploadEvent("/firmware/upload");}
 void start(){server.uploadState.name="firmware";server.uploadState.filename="firmware.bin";server.arguments["firmwareSize"]="4";event(UPLOAD_FILE_START);}
};

void test_page_and_route_inventory(){
    Fixture f;
    TEST_ASSERT_EQUAL(4,f.server.routes.size());
    f.server.request("/firmware",HTTP_GET);
    TEST_ASSERT_EQUAL(200,f.pages.status);
    TEST_ASSERT_NOT_NULL(strstr(f.pages.content.c_str(),"branch&lt;test&gt;"));
    TEST_ASSERT_NOT_NULL(strstr(f.pages.content.c_str(),"/firmware/upload"));}

void test_explicit_restart_flushes_response_first(){
    Fixture f;
    f.server.request("/restart",HTTP_POST);
    TEST_ASSERT_TRUE(f.restart.pending);
    TEST_ASSERT_EQUAL(1,f.restart.executions);
    TEST_ASSERT_TRUE(f.restart.responseBeforeRestart);}

void test_failed_reset_never_restarts(){
    Fixture f;
    f.resetOk=false;
    f.server.request("/factory-reset",HTTP_POST);
    TEST_ASSERT_EQUAL(1,f.resets);
    TEST_ASSERT_EQUAL(400,f.pages.status);
    TEST_ASSERT_EQUAL(0,f.restart.executions);
    TEST_ASSERT_FALSE(f.restart.pending);}

void test_successful_reset_restarts_after_response(){
    Fixture f;
    f.server.request("/factory-reset",HTTP_POST);
    TEST_ASSERT_EQUAL(1,f.resets);
    TEST_ASSERT_TRUE(f.restart.responseBeforeRestart);}

void test_busy_upload_blocks_reset_and_restart(){
    Fixture f;
    f.start();
    f.server.request("/factory-reset",HTTP_POST);
    TEST_ASSERT_EQUAL(400,f.pages.status);
    TEST_ASSERT_EQUAL(0,f.resets);
    f.server.request("/restart",HTTP_POST);
    TEST_ASSERT_EQUAL(400,f.pages.status);
    TEST_ASSERT_EQUAL(0,f.restart.executions);}

void test_successful_upload_stages_without_restart(){
    Fixture f;
    f.start();
    f.server.uploadState.currentSize=4;
    f.event(UPLOAD_FILE_WRITE);
    f.event(UPLOAD_FILE_END);
    f.server.request("/firmware/upload",HTTP_POST);
    TEST_ASSERT_EQUAL(200,f.pages.status);
    TEST_ASSERT_TRUE(f.ota.firmwareStaged());
    TEST_ASSERT_TRUE(f.restart.pending);
    TEST_ASSERT_EQUAL(0,f.restart.executions);
    TEST_ASSERT_NOT_NULL(strstr(f.pages.content.c_str(),webText(f.pages.webLocale(),TextId::RestartLater)));}

void test_invalid_file_is_rejected(){
    Fixture f;
    f.server.uploadState.name="firmware";
    f.server.uploadState.filename="wrong.txt";
    f.event(UPLOAD_FILE_START);
    f.server.request("/firmware/upload",HTTP_POST);
    TEST_ASSERT_EQUAL(400,f.pages.status);
    TEST_ASSERT_EQUAL(0,Update.begins);
    TEST_ASSERT_EQUAL(0,f.restart.executions);}

void test_abort_never_stages_or_restarts(){
    Fixture f;
    f.start();
    f.event(UPLOAD_FILE_ABORTED);
    f.server.request("/firmware/upload",HTTP_POST);
    TEST_ASSERT_EQUAL(400,f.pages.status);
    TEST_ASSERT_FALSE(f.ota.firmwareStaged());
    TEST_ASSERT_EQUAL(0,f.restart.executions);}
int main(){UNITY_BEGIN();
    RUN_TEST(test_page_and_route_inventory);
    RUN_TEST(test_explicit_restart_flushes_response_first);
    RUN_TEST(test_failed_reset_never_restarts);
    RUN_TEST(test_successful_reset_restarts_after_response);
    RUN_TEST(test_busy_upload_blocks_reset_and_restart);
    RUN_TEST(test_successful_upload_stages_without_restart);
    RUN_TEST(test_invalid_file_is_rejected);
    RUN_TEST(test_abort_never_stages_or_restarts);
    return UNITY_END();
}
