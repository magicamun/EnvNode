#include <unity.h>
#include <string>
#include "ConfigurationService.h"
#include "PropertyPreviewWebView.h"
using namespace EnvNode;
HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t length) { return length; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}
void setUp() { Preferences().clear(); Preferences::failStringWrites() = false; }
void tearDown() { Preferences::failStringWrites() = false; }

DisplayConfiguration example() {
    DisplayConfiguration page;
    page.formats[0] = "RainControl <test>";
    page.formats[1] = "Level: %.0f l %.0f%%";
    page.sources[1][0] = "sensor/12/volume";
    page.sources[1][1] = "sensor/12/percentage";
    page.formats[5] = "Valve: %s";
    page.sources[5][0] = "actuator/2/state";
    return page;
}
void test_persistence_reload_and_reset() {
    ConfigurationService service;
    service.loadConfiguration();
    TEST_ASSERT_FALSE(service.getConfiguration().display.configured);
    TEST_ASSERT_TRUE(service.setDeviceName("Keep me"));
    TEST_ASSERT_TRUE(service.setDisplayConfiguration(example()));
    ConfigurationService loaded;
    loaded.loadConfiguration();
    const auto& page = loaded.getConfiguration().display;
    TEST_ASSERT_TRUE(page.configured);
    TEST_ASSERT_EQUAL_STRING("RainControl <test>", page.formats[0].c_str());
    TEST_ASSERT_EQUAL_STRING("sensor/12/percentage", page.sources[1][1].c_str());
    TEST_ASSERT_EQUAL_STRING("", page.formats[4].c_str());
    TEST_ASSERT_EQUAL_STRING("Valve: %s", page.formats[5].c_str());
    TEST_ASSERT_EQUAL_STRING("Keep me", loaded.getConfiguration().device.name.c_str());
    TEST_ASSERT_TRUE(loaded.resetToDefaults());
    loaded.loadConfiguration();
    TEST_ASSERT_FALSE(loaded.getConfiguration().display.configured);
}
void test_invalid_and_failed_writes_preserve_previous_page() {
    ConfigurationService service;
    service.loadConfiguration();
    TEST_ASSERT_TRUE(service.setDisplayConfiguration(example()));
    const String stored = Preferences::storedString("display_page");
    auto page = example();
    page.formats[0] = "%n";
    TEST_ASSERT_FALSE(service.setDisplayConfiguration(page));
    page = example(); page.sources[1][0] = "";
    TEST_ASSERT_FALSE(service.setDisplayConfiguration(page));
    page = example(); page.sources[1][0] = "sensor/0/volume";
    TEST_ASSERT_FALSE(service.setDisplayConfiguration(page));
    page = example(); page.formats[0] = "bad\nline";
    TEST_ASSERT_FALSE(service.setDisplayConfiguration(page));
    page = example(); page.formats[0] = std::string(129, 'a').c_str();
    TEST_ASSERT_FALSE(service.setDisplayConfiguration(page));
    page = example(); page.formats[0] = "Replacement";
    Preferences::failStringWrites() = true;
    TEST_ASSERT_FALSE(service.setDisplayConfiguration(page));
    TEST_ASSERT_EQUAL_STRING(stored.c_str(), Preferences::storedString("display_page").c_str());
    TEST_ASSERT_EQUAL_STRING("RainControl <test>", service.getConfiguration().display.formats[0].c_str());
}
void test_codec_rejects_corruption_and_preserves_destination() {
    String encoded;
    TEST_ASSERT_TRUE(encodeDisplayConfiguration(example(), encoded));
    DisplayConfiguration destination;
    destination.formats[0] = "Unchanged";
    TEST_ASSERT_FALSE(decodeDisplayConfiguration("2\n", destination));
    TEST_ASSERT_FALSE(decodeDisplayConfiguration(encoded + "extra", destination));
    const std::string truncated(encoded.c_str(), encoded.length() - 1);
    TEST_ASSERT_FALSE(decodeDisplayConfiguration(truncated.c_str(), destination));
    TEST_ASSERT_EQUAL_STRING("Unchanged", destination.formats[0].c_str());
    Preferences().putString("display_page", "corrupt");
    ConfigurationService service;
    service.loadConfiguration();
    TEST_ASSERT_FALSE(service.getConfiguration().display.configured);
}
void test_empty_page_is_saved_intentionally_and_limits_roundtrip() {
    ConfigurationService service;
    service.loadConfiguration();
    TEST_ASSERT_TRUE(service.setDisplayConfiguration(DisplayConfiguration{}));
    service.loadConfiguration();
    TEST_ASSERT_TRUE(service.getConfiguration().display.configured);
    TEST_ASSERT_TRUE(service.getConfiguration().display.formats[0].isEmpty());
    DisplayConfiguration page;
    for (size_t line = 0; line < DisplayLineCount; ++line) {
        page.formats[line] = (std::string(120, 'x') + "%s%s%s%s").c_str();
        for (auto& source : page.sources[line]) source = (std::string("controller/65535/") + std::string(64, 'k')).c_str();
    }
    TEST_ASSERT_TRUE(service.setDisplayConfiguration(page));
    service.loadConfiguration();
    TEST_ASSERT_EQUAL_UINT32(128, service.getConfiguration().display.formats[5].length());
    TEST_ASSERT_EQUAL_STRING(page.sources[5][3].c_str(), service.getConfiguration().display.sources[5][3].c_str());
}
void test_v1_migrates_without_enabling_hardware() {
    String encoded;
    TEST_ASSERT_TRUE(encodeDisplayConfiguration(example(), encoded));
    const String legacy = (std::string("1\n") + std::string(encoded.c_str()).substr(9)).c_str();
    Preferences().putString("display_page", legacy);
    ConfigurationService service;
    service.loadConfiguration();
    TEST_ASSERT_TRUE(service.getConfiguration().display.configured);
    TEST_ASSERT_FALSE(service.getConfiguration().display.hardware.enabled);
    TEST_ASSERT_EQUAL_STRING("Valve: %s", service.getConfiguration().display.formats[5].c_str());
    auto page = service.getConfiguration().display;
    page.hardware.enabled = true;
    page.hardware.bus = I2CBus::I2C1;
    page.hardware.address = 0x3D;
    TEST_ASSERT_TRUE(service.setDisplayConfiguration(page));
    service.loadConfiguration();
    TEST_ASSERT_TRUE(service.getConfiguration().display.hardware.enabled);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(I2CBus::I2C1), static_cast<int>(service.getConfiguration().display.hardware.bus));
    TEST_ASSERT_EQUAL_HEX8(0x3D, service.getConfiguration().display.hardware.address);
    TEST_ASSERT_EQUAL_STRING("Valve: %s", service.getConfiguration().display.formats[5].c_str());
}
void test_hardware_parameters_are_strict_and_atomic() {
    TextDisplayConfiguration hardware;
    TEST_ASSERT_FALSE(parseTextDisplayConfiguration("yes", "0", "60", hardware));
    TEST_ASSERT_FALSE(parseTextDisplayConfiguration("1", "2", "60", hardware));
    TEST_ASSERT_FALSE(parseTextDisplayConfiguration("1", "0", "63", hardware));
    TEST_ASSERT_FALSE(hardware.enabled);
    TEST_ASSERT_TRUE(parseTextDisplayConfiguration("1", "1", "61", hardware));
    auto page = example();
    page.hardware = hardware;
    TEST_ASSERT_TRUE(validateDisplayConfiguration(page));
    page.hardware.address = 0x50;
    TEST_ASSERT_FALSE(validateDisplayConfiguration(page));
    page.hardware.address = 0x3C;
    page.hardware.bus = static_cast<I2CBus>(2);
    TEST_ASSERT_FALSE(validateDisplayConfiguration(page));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_v1_migrates_without_enabling_hardware);
    RUN_TEST(test_hardware_parameters_are_strict_and_atomic);
    RUN_TEST(test_persistence_reload_and_reset);
    RUN_TEST(test_invalid_and_failed_writes_preserve_previous_page);
    RUN_TEST(test_codec_rejects_corruption_and_preserves_destination);
    RUN_TEST(test_empty_page_is_saved_intentionally_and_limits_roundtrip);
    return UNITY_END();
}
