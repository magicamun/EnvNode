// EnvNode: see repository LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
#include <unity.h>
#include "WebHost.h"
#include "WebPageFrame.h"
#include "WebStyle.h"
#include <string>
using namespace EnvNode;
HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t n) { return n; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}
void setUp() {}
void tearDown() {}
class TestWiFi : public IWiFiService {
public:
    bool online = false, setup = false;
    void begin() override {}
    void loop() override {}
    bool connected() const override { return online; }
    bool inSetupAccessPointMode() const override { return setup; }
    String ipAddress() const override { return "192.168.1.20"; }
    String hostname() const override { return "Fixture"; }
    int32_t rssi() const override { return -42; }
};
class TestLogger : public ILogger {
public:
    std::string lastMessage;
    void begin(unsigned long) override {}
    void println(const char*) override {}
    void printf(const char*, ...) override {}
    void log(LogLevel, const char* value) override { lastMessage = value; }
};
class FixtureRoutes : public IWebRoutes {
public:
    void registerRoutes(WebServer& server) override {
        server.on("/test/start", HTTP_POST, [&server]() { server.send(202, "text/plain", "started"); });
    }
};
class ExtraRoutes : public IWebRoutes {
public:
    void registerRoutes(WebServer& server) override {
        server.on("/identity", HTTP_GET, [&server]() { server.send(200, "text/plain", "fixture"); });
    }
};
void contains(const String& text, const char* expected) { TEST_ASSERT_NOT_NULL(std::strstr(text.c_str(), expected)); }
void test_host_accepts_independent_route_modules_and_services_clients() {
    TestWiFi wifi; TestLogger logger; WebHost host(logger, wifi, 8080);
    FixtureRoutes fixture; ExtraRoutes extra; host.registerRoutes(fixture); host.registerRoutes(extra); host.begin();
    TEST_ASSERT_TRUE(host.server().started); TEST_ASSERT_EQUAL(8080, host.server().port);
    TEST_ASSERT_TRUE(host.server().request("/test/start", HTTP_POST));
    TEST_ASSERT_EQUAL(202, host.server().responseStatus); TEST_ASSERT_EQUAL_STRING("started", host.server().responseBody.c_str());
    TEST_ASSERT_FALSE(host.server().request("/test/start", HTTP_GET));
    TEST_ASSERT_TRUE(host.server().request("/identity", HTTP_GET));
    host.loop(); TEST_ASSERT_EQUAL(1, host.server().handled);
}
void test_shared_style_keeps_cache_policy_and_content() {
    TestWiFi wifi; TestLogger logger; WebHost host(logger, wifi); host.begin();
    TEST_ASSERT_TRUE(host.server().request("/style.css", HTTP_GET));
    TEST_ASSERT_EQUAL(200, host.server().responseStatus); TEST_ASSERT_EQUAL_STRING("text/css", host.server().responseType.c_str());
    TEST_ASSERT_EQUAL_STRING("public, max-age=86400", host.server().cacheControl.c_str());
    TEST_ASSERT_EQUAL(sharedWebStyleLength(), host.server().responseBody.size());
    TEST_ASSERT_NOT_NULL(std::strstr(host.server().responseBody.c_str(), ".shell"));
}
void test_offline_page_returns_503_without_rendering_header() {
    TestWiFi wifi; TestLogger logger; WebHost host(logger, wifi); int renders = 0;
    host.sendPage([&]() { ++renders; return String("header"); }, "content", 200, "offline", "render error");
    TEST_ASSERT_EQUAL(0, renders); TEST_ASSERT_EQUAL(503, host.server().responseStatus);
    TEST_ASSERT_EQUAL_STRING("offline", host.server().responseBody.c_str());
}
void checkStreaming(bool setup) {
    TestWiFi wifi; wifi.setup = setup; wifi.online = !setup;
    TestLogger logger; WebHost host(logger, wifi);
    host.sendPage([]() { return String("<header>Ü</header>"); }, "<article>content</article>", 400, "offline", "render error");
    TEST_ASSERT_TRUE(host.administrationAvailable());
    TEST_ASSERT_EQUAL(400, host.server().responseStatus);
    TEST_ASSERT_EQUAL_STRING("text/html; charset=utf-8", host.server().responseType.c_str());
    TEST_ASSERT_EQUAL_STRING("<header>Ü</header><article>content</article></main></div></body></html>", host.server().responseBody.c_str());
    TEST_ASSERT_EQUAL(host.server().responseBody.size(), host.server().declaredLength);
    TEST_ASSERT_EQUAL(3, host.server().chunks.size());
}
void test_station_page_streams_header_content_footer_with_exact_length() { checkStreaming(false); }
void test_setup_ap_serves_pages_without_station_connection() { checkStreaming(true); }
void test_empty_header_or_content_returns_render_failure() {
    TestWiFi wifi; wifi.online = true; TestLogger logger; WebHost host(logger, wifi);
    host.sendPage([]() { return String(); }, "content", 200, "offline", "render error");
    TEST_ASSERT_EQUAL(503, host.server().responseStatus); TEST_ASSERT_EQUAL_STRING("render error", host.server().responseBody.c_str());
    TEST_ASSERT_NOT_NULL(std::strstr(logger.lastMessage.c_str(), "Web page rendering failed"));
    host.sendPage([]() { return String("header"); }, "", 200, "offline", "render error");
    TEST_ASSERT_EQUAL(503, host.server().responseStatus);
}
void test_page_frame_supports_fixture_branding_and_application_fragments() {
    const String name = "Test station";
    WebPageFrameData page = {"Check connector", name, "BoardTester", "0.7.0", "build-1", "en",
        []() { return String("<script>fixtureUi()</script>"); },
        []() { return String("<nav><a href='/test'>Test</a></nav>"); },
        []() { return String("<div class='notice'>Stopped</div>"); },
        true, "Setup mode", true};
    const String html = buildWebPageHeader(page);
    contains(html, "<title>Check connector · BoardTester</title>");
    contains(html, "<div class='brand'>Test station</div>");
    contains(html, "BoardTester · v0.7.0"); contains(html, "/style.css?build=build-1");
    contains(html, "<nav><a href='/test'>Test</a></nav>"); contains(html, "<script>fixtureUi()</script>");
    contains(html, "<main class='main main-wide'>"); contains(html, "Setup mode"); contains(html, "Stopped");
    TEST_ASSERT_NULL(std::strstr(html.c_str(), "/sensors"));
}
void test_page_frame_escapes_text_and_omits_setup_hint_when_connected() {
    const String name = "<fixture>&";
    WebPageFrameData page = {"<script>", name, "Test&Board", "v'1", "x'y", "en'",
        []() { return String(); }, []() { return String(); }, []() { return String(); }, false, "Setup only", false};
    const String html = buildWebPageHeader(page);
    contains(html, "&lt;script&gt;"); contains(html, "&lt;fixture&gt;&amp;");
    contains(html, "Test&amp;Board"); contains(html, "x&#39;y");
    TEST_ASSERT_NULL(std::strstr(html.c_str(), "Setup only"));
    TEST_ASSERT_NULL(std::strstr(html.c_str(), "main-wide"));
}
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_host_accepts_independent_route_modules_and_services_clients);
    RUN_TEST(test_shared_style_keeps_cache_policy_and_content);
    RUN_TEST(test_offline_page_returns_503_without_rendering_header);
    RUN_TEST(test_station_page_streams_header_content_footer_with_exact_length);
    RUN_TEST(test_setup_ap_serves_pages_without_station_connection);
    RUN_TEST(test_empty_header_or_content_returns_render_failure);
    RUN_TEST(test_page_frame_supports_fixture_branding_and_application_fragments);
    RUN_TEST(test_page_frame_escapes_text_and_omits_setup_hint_when_connected);
    return UNITY_END();
}
