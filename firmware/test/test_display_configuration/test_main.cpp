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
    const String stored = Preferences::storedString("display_page4");
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
    TEST_ASSERT_EQUAL_STRING(stored.c_str(), Preferences::storedString("display_page4").c_str());
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
    String legacy("1\n");
    const auto old = example();
    for (size_t line = 0; line < DisplayLineCount; ++line) {
        legacy += old.formats[line].c_str(); legacy += '\n';
        for (const auto& source : old.sources[line]) { legacy += source.c_str(); legacy += '\n'; }
    }
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

void test_v2_migration_and_v3_labels_survive_reload() {
    String v2("2\n1\n1\n61\n");
    const auto old = example();
    for (size_t line = 0; line < DisplayLineCount; ++line) {
        v2 += old.formats[line].c_str(); v2 += '\n';
        for (const auto& source : old.sources[line]) { v2 += source.c_str(); v2 += '\n'; }
    }
    Preferences().putString("display_page", v2);
    ConfigurationService service; service.loadConfiguration();
    auto page = service.getConfiguration().display;
    TEST_ASSERT_TRUE(page.hardware.enabled);
    TEST_ASSERT_EQUAL_HEX8(0x3D, page.hardware.address);
    TEST_ASSERT_TRUE(page.labels[5][0].trueText.isEmpty());
    page.labels[5][0].trueText = "Zisterne"; page.labels[5][0].falseText = "Hauswasser";
    TEST_ASSERT_TRUE(service.setDisplayConfiguration(page));
    service.loadConfiguration();
    TEST_ASSERT_EQUAL_STRING("Zisterne", service.getConfiguration().display.labels[5][0].trueText.c_str());
    TEST_ASSERT_EQUAL_STRING("Hauswasser", service.getConfiguration().display.labels[5][0].falseText.c_str());
    TEST_ASSERT_TRUE(service.getConfiguration().display.hardware.enabled);
    page.labels[5][0].trueText = std::string(17, 'x').c_str();
    TEST_ASSERT_FALSE(service.setDisplayConfiguration(page));
    page.labels[5][0].trueText = "bad\nlabel";
    TEST_ASSERT_FALSE(service.setDisplayConfiguration(page));
    page.labels[5][0].trueText = "Zisterne"; page.sources[5][0] = "";
    TEST_ASSERT_FALSE(service.setDisplayConfiguration(page));
    service.loadConfiguration();
    TEST_ASSERT_EQUAL_STRING("Zisterne", service.getConfiguration().display.labels[5][0].trueText.c_str());
}
void test_largest_page_still_fits_one_nvs_string() {
    DisplayConfiguration page;
    for (size_t line = 0; line < DisplayLineCount; ++line) {
        page.formats[line] = (std::string(120, 'x') + "%s%s%s%s").c_str();
        for (size_t source = 0; source < MaxPropertySourcesPerLine; ++source) {
            page.sources[line][source] = (std::string("controller/65535/") + std::string(64, 'k')).c_str();
            page.labels[line][source].trueText = std::string(16, 't').c_str();
            page.labels[line][source].falseText = std::string(16, 'f').c_str();
        }
    }
    String encoded;
    TEST_ASSERT_TRUE(encodeDisplayConfiguration(page, encoded));
    TEST_ASSERT_TRUE(encoded.length() < 4000);
    DisplayConfiguration decoded;
    TEST_ASSERT_TRUE(decodeDisplayConfiguration(encoded, decoded));
    TEST_ASSERT_EQUAL_STRING(page.labels[5][3].falseText.c_str(), decoded.labels[5][3].falseText.c_str());
}

void test_v3_migrates_and_blob_persists_large_enum_page_atomically() {
    auto page=example(); String old("3\n1\n0\n60\n");
    page.labels[5][0].trueText="Zisterne";
    for (size_t line=0;line<DisplayLineCount;++line) {
        old+=page.formats[line].c_str(); old+='\n';
        for(size_t source=0;source<MaxPropertySourcesPerLine;++source) {
            old+=page.sources[line][source].c_str(); old+='\n';
            old+=page.labels[line][source].trueText.c_str(); old+='\n';
            old+=page.labels[line][source].falseText.c_str(); old+='\n';
        }
    }
    Preferences().putString("display_page",old);
    ConfigurationService service; service.loadConfiguration();
    TEST_ASSERT_TRUE(service.getConfiguration().display.hardware.enabled);
    TEST_ASSERT_EQUAL_STRING("Zisterne",service.getConfiguration().display.labels[5][0].trueText.c_str());
    page=service.getConfiguration().display;
    for(size_t line=0;line<DisplayLineCount;++line) {
        page.formats[line]=(std::string(120,'x')+"%s%s%s%s").c_str();
        for(size_t source=0;source<MaxPropertySourcesPerLine;++source) {
            page.sources[line][source]=(std::string("controller/65535/")+std::string(64,'k')).c_str();
            page.labels[line][source].trueText=std::string(16,'t').c_str();
            page.labels[line][source].falseText=std::string(16,'f').c_str();
        }
    }
    for(size_t i=0;i<MaxDisplayEnumTranslations;++i) {
        DisplayEnumTranslation entry; entry.code=(std::string("state_")+std::to_string(i)).c_str(); entry.text="Halten";
        page.enumTranslations.push_back(entry);
    }
    TEST_ASSERT_TRUE(service.setDisplayConfiguration(page));
    TEST_ASSERT_TRUE(Preferences().getBytesLength("display_page4")>4000);
    service.loadConfiguration();
    TEST_ASSERT_EQUAL_UINT32(MaxDisplayEnumTranslations,service.getConfiguration().display.enumTranslations.size());
    TEST_ASSERT_EQUAL_STRING("Halten",service.getConfiguration().display.enumTranslations[0].text.c_str());
    Preferences::failStringWrites()=true;
    page.enumTranslations[0].text="Changed";
    TEST_ASSERT_FALSE(service.setDisplayConfiguration(page));
    TEST_ASSERT_EQUAL_STRING("Halten",service.getConfiguration().display.enumTranslations[0].text.c_str());
    Preferences::failStringWrites()=false;
    page.enumTranslations.push_back(page.enumTranslations[0]);
    TEST_ASSERT_FALSE(validateDisplayConfiguration(page));
}

void test_driver_type_roundtrip_and_legacy_default() {
    ConfigurationService service; service.loadConfiguration();
    for (unsigned int i = 0; i < 3; ++i) {
        auto page = example(); page.hardware.type = static_cast<TextDisplayType>(i);
        TEST_ASSERT_TRUE(service.setDisplayConfiguration(page));
        service.loadConfiguration();
        TEST_ASSERT_EQUAL_INT(i, static_cast<int>(service.getConfiguration().display.hardware.type));
        TEST_ASSERT_EQUAL_STRING(page.formats[0].c_str(), service.getConfiguration().display.formats[0].c_str());
    }
    auto page = example(); String encoded;
    TEST_ASSERT_TRUE(encodeDisplayConfiguration(page, encoded));
    std::string legacy(encoded.c_str());
    size_t start = 2;
    start = legacy.find('\n', start) + 1;
    start = legacy.find('\n', start) + 1;
    size_t end = legacy.find('\n', start);
    size_t length = std::stoul(legacy.substr(start, end - start));
    legacy = legacy.substr(end + 1, length);
    legacy.replace(0, 4, "4\n");
    DisplayConfiguration decoded; decoded.hardware.type = TextDisplayType::Sh1106;
    TEST_ASSERT_TRUE(decodeDisplayConfiguration(legacy.c_str(), decoded));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(TextDisplayType::Ssd1309), static_cast<int>(decoded.hardware.type));
    std::string invalid(encoded.c_str()); invalid[2] = '9';
    TEST_ASSERT_FALSE(decodeDisplayConfiguration(invalid.c_str(), decoded));
    page.hardware.type = static_cast<TextDisplayType>(9);
    TEST_ASSERT_FALSE(service.setDisplayConfiguration(page));
    TextDisplayType type = TextDisplayType::Sh1106;
    TEST_ASSERT_FALSE(parseTextDisplayType("garbage", type));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(TextDisplayType::Sh1106), static_cast<int>(type));
}

void test_two_pages_persist_and_bus_conflicts_are_rejected() {
    ConfigurationService service; service.loadConfiguration();
    auto page = example(); page.hardware.enabled = true;
    page.secondHardware.enabled = true; page.secondHardware.bus = I2CBus::I2C1;
    page.secondHardware.type = TextDisplayType::Sh1106;
    page.secondPage.formats[0] = "Second page";
    page.pageAssignment[0] = 1; page.pageAssignment[1] = 0;
    TEST_ASSERT_TRUE(service.setDisplayConfiguration(page));
    service.loadConfiguration();
    const auto& saved = service.getConfiguration().display;
    TEST_ASSERT_TRUE(saved.secondHardware.enabled);
    TEST_ASSERT_EQUAL_STRING("Second page", saved.secondPage.formats[0].c_str());
    TEST_ASSERT_EQUAL_INT(1, saved.pageAssignment[0]);
    page.secondHardware.bus = page.hardware.bus;
    TEST_ASSERT_FALSE(service.setDisplayConfiguration(page));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(I2CBus::I2C1), static_cast<int>(saved.secondHardware.bus));
    page.secondHardware.address = 0x3D;
    TEST_ASSERT_TRUE(service.setDisplayConfiguration(page));
    page.pageAssignment[1] = 2;
    TEST_ASSERT_FALSE(service.setDisplayConfiguration(page));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_two_pages_persist_and_bus_conflicts_are_rejected);
    RUN_TEST(test_driver_type_roundtrip_and_legacy_default);
    RUN_TEST(test_v3_migrates_and_blob_persists_large_enum_page_atomically);
    RUN_TEST(test_v2_migration_and_v3_labels_survive_reload);
    RUN_TEST(test_largest_page_still_fits_one_nvs_string);
    RUN_TEST(test_v1_migrates_without_enabling_hardware);
    RUN_TEST(test_hardware_parameters_are_strict_and_atomic);
    RUN_TEST(test_persistence_reload_and_reset);
    RUN_TEST(test_invalid_and_failed_writes_preserve_previous_page);
    RUN_TEST(test_codec_rejects_corruption_and_preserves_destination);
    RUN_TEST(test_empty_page_is_saved_intentionally_and_limits_roundtrip);
    return UNITY_END();
}
