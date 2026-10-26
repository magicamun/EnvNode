// EnvNode: see repository LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
#include <unity.h>
#include "NetworkWebRoutes.h"
#include "LogWebRoutes.h"
#include "RecentLogStore.h"
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
    bool online = true, setup = false;
    NetworkAddressSnapshot addresses;
    void begin() override {}
    void loop() override {}
    bool connected() const override { return online; }
    bool inSetupAccessPointMode() const override { return setup; }
    String ipAddress() const override { return addresses.ipAddress; }
    String hostname() const override { return "Node"; }
    int32_t rssi() const override { return -42; }
    NetworkAddressSnapshot stationAddresses() const override { return addresses; }
};
class TestLogger : public ILogger {
public:
    void begin(unsigned long) override {}
    void println(const char*) override {}
    void printf(const char*, ...) override {}
};
class Settings : public INetworkSettings {
public:
    NetworkConfiguration value, requested;
    int writes = 0;
    bool accepted = true, changedPassword = false;
    Settings() { value.hostname = "Node"; value.wifiSSID = "Lab"; value.wifiPassword = "old-secret"; }
    const NetworkConfiguration& getNetworkConfiguration() const override { return value; }
    bool setNetworkConfiguration(const NetworkConfiguration& n, bool password) override {
        ++writes; requested = n; changedPassword = password;
        if (accepted) value = n;
        return accepted;
    }
};
class Pages : public IWebPagePresenter {
public:
    Locale locale = Locale::EnglishUnitedKingdom;
    String title, route, content;
    int status = 0;
    Locale webLocale() const override { return locale; }
    void sendPage(const char* t, const char* r, const String& c, int s, bool) override {
        title = t; route = r; content = c; status = s;
    }
};
class Observer : public INetworkSaveObserver {
public:
    int calls = 0;
    bool success = false;
    void networkSettingsSaved(bool value) override { ++calls; success = value; }
};
void contains(const String& text, const char* expected) { TEST_ASSERT_NOT_NULL(std::strstr(text.c_str(), expected)); }
void form(WebServer& server) {
    server.arguments = {{"hostname", "New-Node"}, {"wifiSSID", "New-Lab"}, {"addressMode", "dhcp"}};
}
void test_network_form_escapes_values_and_never_embeds_saved_password() {
    Settings settings; settings.value.hostname = "Node'<>"; settings.value.wifiSSID = "<Lab>&";
    TestLogger logger; TestWiFi wifi; Pages pages; Observer observer; WebHost host(logger, wifi);
    NetworkWebRoutes routes(host, settings, wifi, pages, observer); host.registerRoutes(routes);
    TEST_ASSERT_TRUE(host.server().request("/network", HTTP_GET));
    TEST_ASSERT_EQUAL_STRING("/network", pages.route.c_str());
    contains(pages.content, "Node&#39;&lt;&gt;"); contains(pages.content, "&lt;Lab&gt;&amp;");
    contains(pages.content, "action='/network/save'"); contains(pages.content, "type='password'");
    TEST_ASSERT_NULL(std::strstr(pages.content.c_str(), "old-secret"));
}
void test_network_form_uses_current_locale_and_assigned_addresses() {
    Settings settings; TestLogger logger; TestWiFi wifi; Pages pages; Observer observer; WebHost host(logger, wifi);
    wifi.addresses.ipAddress = "192.168.1.20"; wifi.addresses.dns1 = "0.0.0.0";
    NetworkWebRoutes routes(host, settings, wifi, pages, observer); host.registerRoutes(routes);
    host.server().request("/network", HTTP_GET); TEST_ASSERT_EQUAL_STRING("Network", pages.title.c_str());
    contains(pages.content, "readonly value='192.168.1.20'"); contains(pages.content, "readonly value='—'");
    pages.locale = Locale::GermanGermany; host.server().request("/network", HTTP_GET);
    TEST_ASSERT_EQUAL_STRING(webText(Locale::GermanGermany, TextId::Network), pages.title.c_str());
}
void test_network_save_preserves_password_without_explicit_checkbox() {
    Settings settings; TestLogger logger; TestWiFi wifi; Pages pages; Observer observer; WebHost host(logger, wifi);
    NetworkWebRoutes routes(host, settings, wifi, pages, observer); host.registerRoutes(routes);
    form(host.server()); host.server().arguments["wifiPassword"] = "must-not-be-applied";
    TEST_ASSERT_TRUE(host.server().request("/network/save", HTTP_POST));
    TEST_ASSERT_EQUAL(1, settings.writes); TEST_ASSERT_FALSE(settings.changedPassword);
    TEST_ASSERT_EQUAL_STRING("old-secret", settings.value.wifiPassword.c_str());
    TEST_ASSERT_EQUAL_STRING("New-Lab", settings.value.wifiSSID.c_str());
    TEST_ASSERT_EQUAL(1, observer.calls); TEST_ASSERT_TRUE(observer.success);
    TEST_ASSERT_FALSE(host.server().request("/network/save", HTTP_GET));
}
void test_network_save_can_explicitly_clear_password_and_pass_static_fields() {
    Settings settings; TestLogger logger; TestWiFi wifi; Pages pages; Observer observer; WebHost host(logger, wifi);
    NetworkWebRoutes routes(host, settings, wifi, pages, observer); host.registerRoutes(routes);
    form(host.server()); host.server().arguments["changeWifiPassword"] = "1";
    host.server().arguments["wifiPassword"] = ""; host.server().arguments["addressMode"] = "static";
    host.server().arguments["ipv4Address"] = "192.168.1.20"; host.server().arguments["subnetMask"] = "255.255.255.0";
    host.server().arguments["gateway"] = "192.168.1.1"; host.server().arguments["dns1"] = "1.1.1.1";
    host.server().arguments["dns2"] = "8.8.8.8"; host.server().request("/network/save", HTTP_POST);
    TEST_ASSERT_TRUE(settings.changedPassword); TEST_ASSERT_TRUE(settings.value.wifiPassword.isEmpty());
    TEST_ASSERT_EQUAL(static_cast<int>(NetworkAddressMode::Static), static_cast<int>(settings.value.addressMode));
    TEST_ASSERT_EQUAL_STRING("192.168.1.20", settings.value.ipv4Address.c_str());
    TEST_ASSERT_EQUAL_STRING("255.255.255.0", settings.value.subnetMask.c_str());
    TEST_ASSERT_EQUAL_STRING("192.168.1.1", settings.value.gateway.c_str());
    TEST_ASSERT_EQUAL_STRING("1.1.1.1", settings.value.dns1.c_str());
    TEST_ASSERT_EQUAL_STRING("8.8.8.8", settings.value.dns2.c_str());
}
void test_failed_network_save_notifies_application_without_changing_settings() {
    Settings settings; settings.accepted = false; TestLogger logger; TestWiFi wifi; Pages pages; Observer observer; WebHost host(logger, wifi);
    NetworkWebRoutes routes(host, settings, wifi, pages, observer); host.registerRoutes(routes);
    form(host.server()); host.server().request("/network/save", HTTP_POST);
    TEST_ASSERT_EQUAL(1, observer.calls); TEST_ASSERT_FALSE(observer.success);
    TEST_ASSERT_EQUAL_STRING("Node", settings.value.hostname.c_str());
}
void test_log_json_rejects_offline_access_and_serves_setup_ap() {
    TestLogger logger; TestWiFi wifi; Pages pages; RecentLogStore logs; WebHost host(logger, wifi);
    LogWebRoutes routes(host, pages, logs); host.registerRoutes(routes);
    wifi.online = false; host.server().request("/logs/data", HTTP_GET);
    TEST_ASSERT_EQUAL(503, host.server().responseStatus);
    TEST_ASSERT_EQUAL_STRING("{\"error\":\"Administration unavailable\"}", host.server().responseBody.c_str());
    wifi.setup = true; host.server().request("/logs/data", HTTP_GET);
    TEST_ASSERT_EQUAL(200, host.server().responseStatus);
    TEST_ASSERT_EQUAL_STRING("application/json; charset=utf-8", host.server().responseType.c_str());
    TEST_ASSERT_EQUAL_STRING("{\"count\":0,\"capacity\":64,\"entries\":[]}", host.server().responseBody.c_str());
}
void test_log_page_and_json_keep_entries_and_localized_page_title() {
    TestLogger logger; TestWiFi wifi; Pages pages; RecentLogStore logs; WebHost host(logger, wifi);
    LogEntry entry; entry.sequence = 7; entry.level = LogLevel::Warn;
    std::snprintf(entry.message, sizeof(entry.message), "<warning>"); logs.append(entry);
    LogWebRoutes routes(host, pages, logs); host.registerRoutes(routes);
    pages.locale = Locale::GermanGermany; host.server().request("/logs", HTTP_GET);
    TEST_ASSERT_EQUAL_STRING("/logs", pages.route.c_str());
    TEST_ASSERT_EQUAL_STRING(webText(pages.locale, TextId::Logs), pages.title.c_str());
    contains(pages.content, "&lt;warning&gt;"); contains(pages.content, "#7");
    host.server().request("/logs/data", HTTP_GET);
    TEST_ASSERT_NOT_NULL(std::strstr(host.server().responseBody.c_str(), "<warning>"));
}
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_network_form_escapes_values_and_never_embeds_saved_password);
    RUN_TEST(test_network_form_uses_current_locale_and_assigned_addresses);
    RUN_TEST(test_network_save_preserves_password_without_explicit_checkbox);
    RUN_TEST(test_network_save_can_explicitly_clear_password_and_pass_static_fields);
    RUN_TEST(test_failed_network_save_notifies_application_without_changing_settings);
    RUN_TEST(test_log_json_rejects_offline_access_and_serves_setup_ap);
    RUN_TEST(test_log_page_and_json_keep_entries_and_localized_page_title);
    return UNITY_END();
}
