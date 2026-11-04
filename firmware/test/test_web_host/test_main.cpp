// EnvNode: see repository LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
#include <unity.h>
#include "WebHost.h"
#include "WebPageFrame.h"
#include "WebStyle.h"
#include "IWebPagePresenter.h"
#include <string>
#include <deque>
#include "HttpClientGate.h"
using namespace EnvNode;
HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t n) { return n; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}
void setUp() {arduinoTestMillis()=0;}
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
void finishPage(WebHost& host) {
    for(unsigned i=0;i<1000 && host.server().clientState.connected();++i)host.loop();
    TEST_ASSERT_FALSE(host.server().clientState.connected());
}
std::string responseBody(WebHost& host) {
    const auto& wire=host.server().clientState.state->sent;
    const auto end=wire.find("\r\n\r\n");TEST_ASSERT_TRUE(end!=std::string::npos);
    return wire.substr(end+4);
}
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
    finishPage(host);const auto body=responseBody(host);
    TEST_ASSERT_EQUAL(sharedWebStyleLength(),body.size());
    TEST_ASSERT_NOT_NULL(std::strstr(body.c_str(), ".shell"));
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
    finishPage(host);const auto body=responseBody(host);
    TEST_ASSERT_EQUAL_STRING("<header>Ü</header><article>content</article></main></div></body></html>",body.c_str());
    TEST_ASSERT_EQUAL(body.size(), host.server().declaredLength);
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
void test_backpressure_keeps_page_and_returns_to_measurement_loop() {
    TestWiFi wifi;wifi.online=true;TestLogger logger;WebHost host(logger,wifi);
    auto& server=host.server();server.clientState.state->blocked=true;
    String body;for(unsigned i=0;i<13000;++i)body+='x';
    server.on("/slow",HTTP_GET,[&]() {
        host.sendPage([](){delay(75);return String("header");},body,200,"offline","error");
    });
    server.onHandleClient=[&]() {server.request("/slow",HTTP_GET);};
    host.loop();server.onHandleClient=nullptr;
    TEST_ASSERT_EQUAL_UINT(75,millis());TEST_ASSERT_TRUE(server.clientState.state->sent.empty());
    const unsigned handled=server.handled;
    unsigned measurementTicks=0;
    for(unsigned i=0;i<20;++i) {delay(250);host.loop();++measurementTicks;}
    TEST_ASSERT_EQUAL_UINT(20,measurementTicks);TEST_ASSERT_EQUAL_UINT(handled,server.handled);
    TEST_ASSERT_TRUE(logger.lastMessage.empty());
    server.clientState.state->blocked=false;server.clientState.state->maxWrite=113;
    finishPage(host);
    const std::string expected="header"+std::string(13000,'x')+webPageFooter();
    TEST_ASSERT_EQUAL_STRING(expected.c_str(),responseBody(host).c_str());
    TEST_ASSERT_NOT_NULL(strstr(logger.lastMessage.c_str(),"HTTP response complete: route=/slow"));
    TEST_ASSERT_NOT_NULL(strstr(logger.lastMessage.c_str(),"blocked_steps=21"));
    TEST_ASSERT_NOT_NULL(strstr(logger.lastMessage.c_str(),"max_gap_ms=250"));
    TEST_ASSERT_NOT_NULL(strstr(logger.lastMessage.c_str(),"max_step_ms=0"));
}
void test_stalled_page_times_out_explicitly_and_next_client_can_be_served() {
    TestWiFi wifi;wifi.online=true;TestLogger logger;WebHost host(logger,wifi);
    host.server().clientState.state->blocked=true;
    host.sendPage([](){return String("header");},"body",200,"offline","error");
    host.loop();delay(10000);host.loop();
    TEST_ASSERT_FALSE(host.server().clientState.connected());
    TEST_ASSERT_NOT_NULL(strstr(logger.lastMessage.c_str(),"HTTP response aborted:"));
    TEST_ASSERT_NOT_NULL(strstr(logger.lastMessage.c_str(),"sent=0"));
    host.server().clientState.state=std::make_shared<WebServer::Client::State>();
    host.sendPage([](){return String("next");},"body",200,"offline","error");
    finishPage(host);TEST_ASSERT_NOT_NULL(strstr(responseBody(host).c_str(),"nextbody"));
}
void test_explicit_restart_waits_for_response_and_grace_without_blocking() {
    TestWiFi wifi;wifi.online=true;TestLogger logger;WebHost host(logger,wifi);bool restarted=false;
    host.sendPage([](){return String("header");},"restart",200,"offline","error");
    host.afterResponse([&](){restarted=true;});
    TEST_ASSERT_FALSE(restarted);finishPage(host);TEST_ASSERT_FALSE(restarted);
    delay(199);host.loop();TEST_ASSERT_FALSE(restarted);
    delay(1);host.loop();TEST_ASSERT_TRUE(restarted);
}
class PumpSink : public IHttpResponseSink {
public:
    bool online=true,blocked=false,fail=false;size_t limit=7,calls=0;uint32_t writeMs=0;std::string sent;
    bool connected() override {return online;}
    int send(const char* data,size_t size) override {
        ++calls;delay(writeMs);if(fail)return -1;if(blocked)return 0;
        const size_t n=std::min(size,limit);sent.append(data,n);return static_cast<int>(n);
    }
};
void test_response_pump_preserves_partial_writes_backpressure_and_bounds_work() {
    HttpResponsePump pump;PumpSink sink;
    TEST_ASSERT_TRUE(pump.begin(String("HTTP\r\n"),String("head"),String("bodyä"),"tail",0));
    sink.blocked=true;TEST_ASSERT_TRUE(pump.step(sink,100)==HttpResponseState::More);
    TEST_ASSERT_EQUAL_UINT(0,pump.sent());TEST_ASSERT_EQUAL_UINT(1,sink.calls);
    sink.blocked=false;
    for(unsigned i=0;i<100 && pump.active();++i) {
        const auto calls=sink.calls;pump.step(sink,200+i);
        TEST_ASSERT_LESS_OR_EQUAL_UINT(HttpResponsePump::StepCalls,sink.calls-calls);
    }
    TEST_ASSERT_EQUAL_STRING("HTTP\r\nheadbodyätail",sink.sent.c_str());
    TEST_ASSERT_EQUAL_UINT(pump.expected(),pump.sent());TEST_ASSERT_FALSE(pump.active());
    String large;for(unsigned i=0;i<20000;++i)large+='x';sink.limit=10000;sink.sent.clear();
    TEST_ASSERT_TRUE(pump.begin(String("H"),String(),std::move(large),"",0));
    pump.step(sink,1);TEST_ASSERT_EQUAL_UINT(HttpResponsePump::StepBytes,pump.sent());TEST_ASSERT_TRUE(pump.active());
}
void test_large_response_uses_fewer_loop_turns_without_waiting() {
    HttpResponsePump pump;PumpSink sink;sink.limit=10000;
    String body;for(unsigned i=0;i<32768;++i)body+='x';
    TEST_ASSERT_TRUE(pump.begin(String("H"),String(),std::move(body),"",0));
    unsigned turns=0;
    while(pump.active() && turns<100) {
        delay(250);pump.step(sink,millis());++turns;
    }
    TEST_ASSERT_EQUAL_UINT(5,turns);
    TEST_ASSERT_EQUAL_UINT(1250,millis());
    TEST_ASSERT_EQUAL_UINT(32769,sink.sent.size());
}
void test_response_pump_yields_on_elapsed_budget_even_across_clock_wrap() {
    for(const uint32_t start : {0U,0xFFFFFFFEU}) {
        arduinoTestMillis()=start;
        HttpResponsePump pump;PumpSink sink;sink.limit=10000;sink.writeMs=1;
        String body;for(unsigned i=0;i<20000;++i)body+='x';
        TEST_ASSERT_TRUE(pump.begin(String("H"),String(),std::move(body),"",start));
        TEST_ASSERT_TRUE(pump.step(sink,start)==HttpResponseState::More);
        TEST_ASSERT_EQUAL_UINT(2,sink.calls);
        TEST_ASSERT_EQUAL_UINT(2,static_cast<uint32_t>(millis()-start));
        TEST_ASSERT_TRUE(pump.active());
        sink.writeMs=0;
        while(pump.active())pump.step(sink,millis());
        TEST_ASSERT_EQUAL_UINT(20001,sink.sent.size());
    }
}
void test_response_pump_reports_socket_failure_disconnect_and_timeout_across_wrap() {
    HttpResponsePump pump;PumpSink sink;
    TEST_ASSERT_TRUE(pump.begin(String("H"),String(),String("body"),"",0));
    sink.fail=true;TEST_ASSERT_TRUE(pump.step(sink,1)==HttpResponseState::WriteFailed);
    TEST_ASSERT_FALSE(pump.active());TEST_ASSERT_EQUAL_UINT(0,pump.sent());
    sink.fail=false;sink.online=false;
    TEST_ASSERT_TRUE(pump.begin(String("H"),String(),String("body"),"",0));
    TEST_ASSERT_TRUE(pump.step(sink,1)==HttpResponseState::Disconnected);
    sink.online=true;sink.blocked=true;const uint32_t start=0xFFFFFF00U;
    TEST_ASSERT_TRUE(pump.begin(String("H"),String(),String("body"),"",start));
    TEST_ASSERT_TRUE(pump.step(sink,start+9999)==HttpResponseState::More);
    TEST_ASSERT_TRUE(pump.step(sink,start+10000)==HttpResponseState::Timeout);
}
void test_response_pump_rejects_oversized_page_before_transmission() {
    HttpResponsePump pump;String huge;for(unsigned i=0;i<HttpResponsePump::MaxBytes;++i)huge+='x';
    TEST_ASSERT_FALSE(pump.begin(String("H"),String(),std::move(huge),"",0));
    TEST_ASSERT_FALSE(pump.active());TEST_ASSERT_EQUAL_UINT(0,pump.sent());
}
void test_slow_request_without_response_is_identified() {
    TestWiFi wifi;TestLogger logger;WebHost host(logger,wifi);
    host.server().onHandleClient=[](){delay(1500);};
    host.loop();
    TEST_ASSERT_NOT_NULL(strstr(logger.lastMessage.c_str(),"route=(no response)"));
    TEST_ASSERT_NOT_NULL(strstr(logger.lastMessage.c_str(),"send_ms=0"));
}
void test_measurements_cover_flash_writes_and_reset_between_requests() {
    MeasuredWebServer server(80);
    server.writeDelayMs=500;server.beginMeasurement();
    server.on("/style",HTTP_GET,[&](){server.send_P(200,"text/css","css",3);});
    server.request("/style",HTTP_GET);
    TEST_ASSERT_EQUAL_UINT(500,server.writeMs());
    TEST_ASSERT_EQUAL_UINT(3,server.writtenBytes());
    TEST_ASSERT_EQUAL_STRING("/style",server.responseRoute().c_str());
    server.beginMeasurement();
    TEST_ASSERT_EQUAL_UINT(0,server.writeMs());
    TEST_ASSERT_EQUAL_UINT(0,server.writtenBytes());
    TEST_ASSERT_TRUE(server.responseRoute().isEmpty());
}
struct GateClient {
    struct State {bool online=true;std::string bytes;};
    std::shared_ptr<State> state;
    static GateClient make(const char* bytes="") {
        GateClient c;c.state=std::make_shared<State>();c.state->bytes=bytes;return c;
    }
    bool connected() const {return state && state->online;}
    size_t available() const {return connected()?state->bytes.size():0;}
    void stop() {if(state)state->online=false;}
};
struct GateAcceptor {
    std::deque<GateClient> queue;unsigned calls=0;
    GateClient available() {
        ++calls;if(queue.empty())return {};
        auto c=queue.front();queue.pop_front();return c;
    }
};
void test_empty_connection_does_not_hold_up_ready_request_and_can_be_used_later() {
    HttpClientGate<GateClient> gate;GateAcceptor server;GateClient selected;
    auto idle=GateClient::make();auto request=GateClient::make("GET /controllers HTTP/1.1\r\n\r\n");
    server.queue.push_back(idle);TEST_ASSERT_FALSE(gate.poll(server,selected,0));
    server.queue.push_back(request);TEST_ASSERT_TRUE(gate.poll(server,selected,300));
    TEST_ASSERT_TRUE(selected.state==request.state);TEST_ASSERT_TRUE(idle.connected());
    TEST_ASSERT_EQUAL_STRING("GET /controllers HTTP/1.1\r\n\r\n",selected.state->bytes.c_str());
    // Even a fragmented request's first byte is preserved for the SDK parser.
    idle.state->bytes="G";TEST_ASSERT_TRUE(gate.poll(server,selected,400));
    TEST_ASSERT_TRUE(selected.state==idle.state);TEST_ASSERT_EQUAL_STRING("G",selected.state->bytes.c_str());
    TEST_ASSERT_EQUAL_UINT(3,server.calls);
}
void test_full_idle_pool_evicts_oldest_but_next_ready_client_does_not_wait() {
    HttpClientGate<GateClient> gate;GateAcceptor server;GateClient selected;
    auto a=GateClient::make(),b=GateClient::make(),c=GateClient::make(),request=GateClient::make("GET /");
    server.queue.push_back(a);TEST_ASSERT_FALSE(gate.poll(server,selected,0));
    server.queue.push_back(b);TEST_ASSERT_FALSE(gate.poll(server,selected,10));
    server.queue.push_back(c);TEST_ASSERT_FALSE(gate.poll(server,selected,20));
    TEST_ASSERT_FALSE(a.connected());TEST_ASSERT_TRUE(b.connected());TEST_ASSERT_TRUE(c.connected());
    TEST_ASSERT_EQUAL_UINT(1,gate.evicted());
    server.queue.push_back(request);TEST_ASSERT_TRUE(gate.poll(server,selected,30));
    TEST_ASSERT_TRUE(selected.state==request.state);TEST_ASSERT_EQUAL_UINT(1,gate.evicted());
}
void test_pending_ready_clients_are_served_before_new_arrivals_without_losing_bytes() {
    HttpClientGate<GateClient> gate;GateAcceptor server;GateClient selected;
    auto a=GateClient::make(),b=GateClient::make(),c=GateClient::make("GET /new");
    server.queue.push_back(a);gate.poll(server,selected,0);
    server.queue.push_back(b);gate.poll(server,selected,1);
    a.state->bytes="GET /a";b.state->bytes="GET /b";server.queue.push_back(c);
    TEST_ASSERT_TRUE(gate.poll(server,selected,2));TEST_ASSERT_TRUE(selected.state==a.state);
    TEST_ASSERT_TRUE(gate.poll(server,selected,3));TEST_ASSERT_TRUE(selected.state==b.state);
    TEST_ASSERT_TRUE(gate.poll(server,selected,4));TEST_ASSERT_TRUE(selected.state==c.state);
    TEST_ASSERT_EQUAL_UINT(0,gate.evicted());TEST_ASSERT_EQUAL_UINT(0,gate.rejected());
    TEST_ASSERT_EQUAL_STRING("GET /a",a.state->bytes.c_str());
    TEST_ASSERT_EQUAL_STRING("GET /b",b.state->bytes.c_str());
}
void test_idle_timeout_wraps_and_ready_bytes_at_deadline_are_not_discarded() {
    HttpClientGate<GateClient> gate;GateAcceptor server;GateClient selected;
    auto idle=GateClient::make();const uint32_t start=0xfffffff0U;
    server.queue.push_back(idle);gate.poll(server,selected,start);
    TEST_ASSERT_FALSE(gate.poll(server,selected,start+4999));TEST_ASSERT_TRUE(idle.connected());
    TEST_ASSERT_FALSE(gate.poll(server,selected,start+5000));TEST_ASSERT_FALSE(idle.connected());
    TEST_ASSERT_EQUAL_UINT(1,gate.expired());
    auto late=GateClient::make();server.queue.push_back(late);gate.poll(server,selected,6000);
    late.state->bytes="G";TEST_ASSERT_TRUE(gate.poll(server,selected,11000));
    TEST_ASSERT_TRUE(selected.state==late.state);TEST_ASSERT_EQUAL_UINT(1,gate.expired());
}
void test_disconnected_waiting_client_releases_capacity() {
    HttpClientGate<GateClient> gate;GateAcceptor server;GateClient selected;
    auto gone=GateClient::make(),next=GateClient::make();server.queue.push_back(gone);
    gate.poll(server,selected,0);gone.stop();server.queue.push_back(next);
    TEST_ASSERT_FALSE(gate.poll(server,selected,1));
    next.state->bytes="GET /";TEST_ASSERT_TRUE(gate.poll(server,selected,2));
    TEST_ASSERT_TRUE(selected.state==next.state);TEST_ASSERT_EQUAL_UINT(0,gate.evicted());
}
void test_static_flash_asset_uses_deferred_pump_without_html_wrapper() {
    TestWiFi wifi;wifi.online=true;TestLogger logger;WebHost host(logger,wifi);
    const std::string script(12000,'x');host.server().clientState.state->blocked=true;
    host.sendStaticContent("application/javascript",script.c_str());host.loop();
    TEST_ASSERT_TRUE(host.server().clientState.state->sent.empty());
    TEST_ASSERT_EQUAL_STRING("no-cache",host.server().cacheControl.c_str());
    host.server().clientState.state->blocked=false;finishPage(host);
    TEST_ASSERT_EQUAL_STRING(script.c_str(),responseBody(host).c_str());
    TEST_ASSERT_EQUAL_UINT(script.size(),host.server().declaredLength);
}
void test_presenter_dispatches_owned_pages_without_const_reference_copy() {
    struct OwnedPages : IWebPagePresenter {
        String owned;unsigned copies=0,moves=0;
        Locale webLocale() const override {return Locale::EnglishUnitedKingdom;}
        void sendPage(const char*,const char*,const String& value,int,bool) override {++copies;owned=value;}
        void sendPage(const char*,const char*,String&& value,int,bool) override {++moves;owned=std::move(value);}
    } pages;
    String body;for(unsigned i=0;i<10000;++i)body+='x';const char* original=body.c_str();
    IWebPagePresenter& presenter=pages;presenter.sendPage("title","/",std::move(body));
    TEST_ASSERT_EQUAL_UINT(1,pages.moves);TEST_ASSERT_EQUAL_UINT(0,pages.copies);
    TEST_ASSERT_TRUE(original==pages.owned.c_str());
}
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_static_flash_asset_uses_deferred_pump_without_html_wrapper);
    RUN_TEST(test_presenter_dispatches_owned_pages_without_const_reference_copy);
    RUN_TEST(test_empty_connection_does_not_hold_up_ready_request_and_can_be_used_later);
    RUN_TEST(test_full_idle_pool_evicts_oldest_but_next_ready_client_does_not_wait);
    RUN_TEST(test_pending_ready_clients_are_served_before_new_arrivals_without_losing_bytes);
    RUN_TEST(test_idle_timeout_wraps_and_ready_bytes_at_deadline_are_not_discarded);
    RUN_TEST(test_disconnected_waiting_client_releases_capacity);
    RUN_TEST(test_backpressure_keeps_page_and_returns_to_measurement_loop);
    RUN_TEST(test_stalled_page_times_out_explicitly_and_next_client_can_be_served);
    RUN_TEST(test_explicit_restart_waits_for_response_and_grace_without_blocking);
    RUN_TEST(test_response_pump_preserves_partial_writes_backpressure_and_bounds_work);
    RUN_TEST(test_large_response_uses_fewer_loop_turns_without_waiting);
    RUN_TEST(test_response_pump_yields_on_elapsed_budget_even_across_clock_wrap);
    RUN_TEST(test_response_pump_reports_socket_failure_disconnect_and_timeout_across_wrap);
    RUN_TEST(test_response_pump_rejects_oversized_page_before_transmission);
    RUN_TEST(test_slow_request_without_response_is_identified);
    RUN_TEST(test_measurements_cover_flash_writes_and_reset_between_requests);
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
