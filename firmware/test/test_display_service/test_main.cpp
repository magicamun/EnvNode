#include <unity.h>
#include <cstring>
#include "DisplayService.h"
#include "TextDisplayLayout.h"
#include "OledCommandTransport.h"
#include <vector>
using namespace EnvNode;
HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t length) { return length; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}
void setUp() {}
void tearDown() {}
struct Clock : IMonotonicClock {
    uint32_t now = 0;
    uint32_t nowMs() const override { return now; }
};
struct Log : ILogger {
    unsigned messages = 0;
    std::string text;
    void begin(unsigned long) override {}
    void println(const char* message) override { ++messages; text += message; text += "\n"; }
    void printf(const char*, ...) override { ++messages; }
};
struct Reader : IPropertyReader {
    bool available = true;
    bool valid = true;
    float value = 12.5F;
    bool boolean = false;
    mutable unsigned reads = 0;
    bool describe(const PropertyReference&, PropertyDescription& description) const override {
        description = PropertyDescription{};
        description.valueKind = boolean ? PropertyValueKind::Boolean : PropertyValueKind::FloatingPoint;
        return true;
    }
    PropertyReadResult read(const PropertyReference&, PropertySnapshot& result) const override {
        ++reads;
        result = PropertySnapshot{};
        if (!available) return PropertyReadResult::NoValue;
        result.valid = valid;
        result.value = boolean ? MeasurementValue::boolean(value != 0) : MeasurementValue::floatingPoint(value);
        return PropertyReadResult::Available;
    }
};
struct Display : ITextDisplay {
    bool beginOk = true, showOk = true;
    unsigned begins = 0, shows = 0, ends = 0;
    std::string operations;
    TextDisplayFrame last;
    TextDisplayConfiguration hardware;
    bool begin(const TextDisplayConfiguration& config) override { operations += "B"; ++begins; hardware = config; return beginOk; }
    bool show(const TextDisplayFrame& frame) override { ++shows; last = frame; return showOk; }
    void end() override { operations += "E"; ++ends; }
};
struct Fixture {
    DisplayConfiguration config;
    Reader reader;
    Display display;
    Clock clock;
    Log log;
    DisplayService service;
    Fixture() : service(config, reader, display, clock, log) {
        config.formats[0] = "RainControl";
        config.formats[1] = "Level: %.1f";
        config.sources[1][0] = "sensor/1/level";
    }
};
void test_disabled_enable_refresh_and_live_edit() {
    Fixture f;
    f.service.loop();
    TEST_ASSERT_EQUAL_UINT32(0, f.display.begins);
    TEST_ASSERT_EQUAL_UINT32(0, f.reader.reads);
    f.config.hardware.enabled = true;
    f.service.loop();
    TEST_ASSERT_EQUAL_UINT32(1, f.display.begins);
    TEST_ASSERT_EQUAL_STRING("Level: 12.5", f.display.last.lines[1]);
    f.clock.now = 999; f.service.loop();
    TEST_ASSERT_EQUAL_UINT32(1, f.display.shows);
    f.clock.now = 1000; f.config.formats[0] = "Updated"; f.reader.value = 20;
    f.service.loop();
    TEST_ASSERT_EQUAL_UINT32(2, f.display.shows);
    TEST_ASSERT_EQUAL_STRING("Updated", f.display.last.lines[0]);
    TEST_ASSERT_EQUAL_STRING("Level: 20.0", f.display.last.lines[1]);
    f.config.hardware.enabled = false; f.service.loop();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(DisplayStatus::Disabled), static_cast<int>(f.service.status()));
    const unsigned shown = f.display.shows;
    f.clock.now += 30000; f.service.loop();
    TEST_ASSERT_EQUAL_UINT32(shown, f.display.shows);
}
void test_missing_display_retries_and_recovers() {
    Fixture f;
    f.config.hardware.enabled = true; f.display.beginOk = false;
    f.service.loop();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(DisplayStatus::Unavailable), static_cast<int>(f.service.status()));
    TEST_ASSERT_EQUAL_UINT32(0, f.reader.reads);
    f.clock.now = 9999; f.service.loop();
    TEST_ASSERT_EQUAL_UINT32(1, f.display.begins);
    f.clock.now = 10000; f.display.beginOk = true; f.service.loop();
    TEST_ASSERT_EQUAL_UINT32(2, f.display.begins);
    TEST_ASSERT_EQUAL_UINT32(1, f.display.shows);
    f.clock.now = 11000; f.display.showOk = false; f.service.loop();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(DisplayStatus::Unavailable), static_cast<int>(f.service.status()));
    f.clock.now = 20999; f.service.loop();
    TEST_ASSERT_EQUAL_UINT32(2, f.display.begins);
    f.clock.now = 21000; f.display.showOk = true; f.service.loop();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(DisplayStatus::Ready), static_cast<int>(f.service.status()));
}
void test_line_errors_clear_stale_text_and_recover() {
    Fixture f; f.config.hardware.enabled = true;
    f.service.loop();
    f.reader.available = false; f.clock.now = 1000; f.service.loop();
    TEST_ASSERT_EQUAL_STRING("[Line 2 error]", f.display.last.lines[1]);
    TEST_ASSERT_EQUAL_STRING("RainControl", f.display.last.lines[0]);
    TEST_ASSERT_EQUAL_STRING("", f.display.last.lines[5]);
    f.reader.available = true; f.clock.now = 2000; f.service.loop();
    TEST_ASSERT_EQUAL_STRING("Level: 12.5", f.display.last.lines[1]);
    f.reader.valid = false; f.clock.now = 3000; f.service.loop();
    TEST_ASSERT_EQUAL_STRING("[Line 2 error]", f.display.last.lines[1]);
}
void test_hardware_change_and_clock_wrap() {
    Fixture f; f.config.hardware.enabled = true;
    f.clock.now = UINT32_MAX - 499; f.service.loop();
    f.clock.now = 499; f.service.loop();
    TEST_ASSERT_EQUAL_UINT32(1, f.display.shows);
    f.clock.now = 500; f.service.loop();
    TEST_ASSERT_EQUAL_UINT32(2, f.display.shows);
    f.config.hardware.bus = I2CBus::I2C1; f.config.hardware.address = 0x3D;
    f.service.loop();
    TEST_ASSERT_EQUAL_UINT32(2, f.display.begins);
    TEST_ASSERT_EQUAL_UINT32(1, f.display.ends);
    TEST_ASSERT_EQUAL_HEX8(0x3D, f.display.hardware.address);
}
void test_saved_translation_reaches_oled_and_updates_live() {
    Fixture f; f.config.hardware.enabled = true; f.reader.boolean = true;
    f.config.formats[1] = "Ventil: %s";
    f.config.labels[1][0].trueText = "Zisterne";
    f.config.labels[1][0].falseText = "Hauswasser";
    f.service.loop();
    TEST_ASSERT_EQUAL_STRING("Ventil: Zisterne", f.display.last.lines[1]);
    f.reader.value = 0; f.clock.now = 1000; f.service.loop();
    TEST_ASSERT_EQUAL_STRING("Ventil: Hauswasser", f.display.last.lines[1]);
    f.config.labels[1][0].falseText = "Leitungswasser";
    f.clock.now = 2000; f.service.loop();
    TEST_ASSERT_EQUAL_STRING("Ventil: Leitungswasse", f.display.last.lines[1]); // 21 complete cells
    TEST_ASSERT_EQUAL_STRING("Leitungswasser", f.config.labels[1][0].falseText.c_str());
    TEST_ASSERT_EQUAL_UINT32(1, f.display.begins);
}

void test_driver_change_restarts_output_and_keeps_page() {
    Fixture f; f.config.hardware.enabled = true; f.service.loop();
    f.config.hardware.type = TextDisplayType::Sh1106; f.service.loop();
    TEST_ASSERT_EQUAL_UINT32(2, f.display.begins);
    TEST_ASSERT_EQUAL_UINT32(1, f.display.ends);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(TextDisplayType::Sh1106), static_cast<int>(f.display.hardware.type));
    TEST_ASSERT_EQUAL_STRING("RainControl", f.display.last.lines[0]);
}
void test_bus_roundtrip_releases_old_target_even_after_frame_failure() {
    Fixture f; f.config.hardware.enabled = true; f.service.loop();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(I2CBus::I2C0), static_cast<int>(f.display.hardware.bus));
    f.config.hardware.bus = I2CBus::I2C1;
    f.config.hardware.type = TextDisplayType::Ssd1306;
    f.service.loop();
    TEST_ASSERT_EQUAL_STRING("BEB", f.display.operations.c_str());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(I2CBus::I2C1), static_cast<int>(f.display.hardware.bus));
    f.clock.now += 1000; f.display.showOk = false; f.service.loop();
    f.config.hardware.bus = I2CBus::I2C0;
    f.config.hardware.type = TextDisplayType::Ssd1309;
    f.display.showOk = true; f.service.loop();
    TEST_ASSERT_EQUAL_STRING("BEBEB", f.display.operations.c_str());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(I2CBus::I2C0), static_cast<int>(f.display.hardware.bus));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(DisplayStatus::Ready), static_cast<int>(f.service.status()));
    TEST_ASSERT_EQUAL_STRING("RainControl", f.display.last.lines[0]);
}
void test_two_displays_render_independent_pages_and_recover_independently() {
    Fixture f; Display second;
    f.config.hardware.enabled = true;
    f.config.secondHardware.enabled = true; f.config.secondHardware.bus = I2CBus::I2C1;
    f.config.secondPage.formats[0] = "Second";
    DisplayService service(f.config, f.reader, f.display, f.clock, f.log, &second);
    second.beginOk = false; service.loop();
    TEST_ASSERT_EQUAL_STRING("RainControl", f.display.last.lines[0]);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(DisplayStatus::Unavailable), static_cast<int>(service.status(1)));
    f.clock.now = 1000; service.loop();
    TEST_ASSERT_EQUAL_UINT32(2, f.display.shows);
    TEST_ASSERT_EQUAL_UINT32(1, second.begins);
    second.beginOk = true; f.clock.now = 10000; service.loop();
    TEST_ASSERT_EQUAL_STRING("Second", second.last.lines[0]);
    f.config.pageAssignment[0] = 1; f.config.pageAssignment[1] = 0;
    f.clock.now += 1000; service.loop();
    TEST_ASSERT_EQUAL_STRING("Second", f.display.last.lines[0]);
    TEST_ASSERT_EQUAL_STRING("RainControl", second.last.lines[0]);
    f.config.secondHardware.enabled = false; f.clock.now += 1000; service.loop();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(DisplayStatus::Ready), static_cast<int>(service.status(0)));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(DisplayStatus::Disabled), static_cast<int>(service.status(1)));
}
void test_ready_is_not_reported_when_first_or_recovery_frame_fails() {
    Fixture f; f.config.hardware.enabled = true; f.display.showOk = false;
    f.service.loop();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(DisplayStatus::Unavailable), static_cast<int>(f.service.status()));
    TEST_ASSERT_EQUAL(std::string::npos, f.log.text.find("ready"));
    f.clock.now = 10000; f.service.loop();
    TEST_ASSERT_EQUAL_UINT32(2, f.display.begins);
    TEST_ASSERT_EQUAL(std::string::npos, f.log.text.find("ready"));
    f.clock.now = 20000; f.display.showOk = true; f.service.loop();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(DisplayStatus::Ready), static_cast<int>(f.service.status()));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, f.log.text.find("full frame transferred"));
}
struct PacketSink : IOledPacketSink {
    std::vector<std::vector<uint8_t>> packets;
    size_t failAt = 0;
    bool sendOledPacket(const uint8_t* bytes, size_t length) override {
        packets.emplace_back(bytes, bytes + length);
        return packets.size() != failAt;
    }
};
void test_oled_packets_keep_command_and_arguments_together_and_abort_on_failure() {
    PacketSink sink; OledCommandTransport transport(sink);
    TEST_ASSERT_TRUE(transport.command(0x20));
    TEST_ASSERT_TRUE(transport.argument(0x02));
    TEST_ASSERT_EQUAL_UINT32(0, sink.packets.size());
    TEST_ASSERT_TRUE(transport.flush());
    const uint8_t expected[] = {0x00, 0x20, 0x02};
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, sink.packets[0].data(), 3);
    uint8_t frame[1024] = {};
    sink.failAt = 4;
    TEST_ASSERT_FALSE(transport.data(frame, sizeof(frame)));
    TEST_ASSERT_EQUAL_UINT32(4, sink.packets.size());
    TEST_ASSERT_FALSE(transport.command(0xAF));
    TEST_ASSERT_EQUAL_UINT32(4, sink.packets.size());
    transport.reset(); sink.failAt = 0;
    TEST_ASSERT_TRUE(transport.data(frame, sizeof(frame)));
    size_t recovered = 0;
    for (size_t i = 4; i < sink.packets.size(); ++i) {
        TEST_ASSERT_EQUAL_HEX8(0x40, sink.packets[i][0]);
        TEST_ASSERT_LESS_OR_EQUAL_UINT32(25, sink.packets[i].size());
        recovered += sink.packets[i].size() - 1;
    }
    TEST_ASSERT_EQUAL_UINT32(1024, recovered);
}
void test_oled_command_state_is_private_to_each_display() {
    PacketSink a, b; OledCommandTransport first(a), second(b);
    first.command(0xD3); second.command(0xA8);
    first.argument(0x00); second.argument(0x3F);
    first.flush(); second.flush();
    const uint8_t offset[] = {0x00, 0xD3, 0x00}, multiplex[] = {0x00, 0xA8, 0x3F};
    TEST_ASSERT_EQUAL_UINT8_ARRAY(offset, a.packets[0].data(), 3);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(multiplex, b.packets[0].data(), 3);
}
void test_font_geometry_clips_complete_utf8_cells() {
    TextDisplayConfiguration config;
    auto layout = textDisplayLayout(config);
    TEST_ASSERT_EQUAL_UINT32(21, layout.columns()); TEST_ASSERT_EQUAL_UINT32(6, layout.rows());
    config.font = TextDisplayFont::Fixed8x13; layout = textDisplayLayout(config);
    TEST_ASSERT_EQUAL_UINT32(16, layout.columns()); TEST_ASSERT_EQUAL_UINT32(4, layout.rows());
    char result[128];
    clipDisplayText("123456789012345äöZ", 16, result, sizeof(result));
    TEST_ASSERT_EQUAL_STRING("123456789012345ä", result);
    clipDisplayText("ÄÖÜäöüß°", 8, result, sizeof(result));
    TEST_ASSERT_EQUAL_STRING("ÄÖÜäöüß°", result);
    clipDisplayText("A😀äB", 3, result, sizeof(result)); TEST_ASSERT_EQUAL_STRING("A?ä", result);
    strcpy(result, "A😀äB"); clipDisplayText(result, 3, result, sizeof(result));
    TEST_ASSERT_EQUAL_STRING("A?ä", result);
    clipDisplayText("ä", 1, result, 2); TEST_ASSERT_EQUAL_STRING("", result);
    clipDisplayText("text", 0, result, sizeof(result)); TEST_ASSERT_EQUAL_STRING("", result);
}
void test_two_fonts_clip_independently_and_switch_restores_full_page() {
    Fixture f; Display second;
    f.config.hardware.enabled = true;
    f.config.secondHardware.enabled = true; f.config.secondHardware.bus = I2CBus::I2C1;
    f.config.secondHardware.font = TextDisplayFont::Fixed8x13;
    f.config.pageAssignment[1] = 0;
    f.config.formats[0] = "123456789012345äöüXYZtail";
    f.config.formats[5] = "Sixth line";
    DisplayService service(f.config, f.reader, f.display, f.clock, f.log, &second);
    service.loop();
    TEST_ASSERT_EQUAL_STRING("123456789012345äöüXYZ", f.display.last.lines[0]);
    TEST_ASSERT_EQUAL_STRING("123456789012345ä", second.last.lines[0]);
    TEST_ASSERT_EQUAL_STRING("Sixth line", f.display.last.lines[5]);
    TEST_ASSERT_EQUAL_STRING("", second.last.lines[5]);
    TEST_ASSERT_EQUAL_STRING("123456789012345äöüXYZtail", f.config.formats[0].c_str());
    f.config.secondHardware.font = TextDisplayFont::Fixed6x10;
    service.loop();
    TEST_ASSERT_EQUAL_UINT32(2, second.begins);
    TEST_ASSERT_EQUAL_STRING("Sixth line", second.last.lines[5]);
    TEST_ASSERT_EQUAL_STRING(f.display.last.lines[0], second.last.lines[0]);
}
int main(int, char**) {
    UNITY_BEGIN(); RUN_TEST(test_font_geometry_clips_complete_utf8_cells); RUN_TEST(test_two_fonts_clip_independently_and_switch_restores_full_page); RUN_TEST(test_ready_is_not_reported_when_first_or_recovery_frame_fails); RUN_TEST(test_oled_packets_keep_command_and_arguments_together_and_abort_on_failure); RUN_TEST(test_oled_command_state_is_private_to_each_display); RUN_TEST(test_two_displays_render_independent_pages_and_recover_independently); RUN_TEST(test_bus_roundtrip_releases_old_target_even_after_frame_failure); RUN_TEST(test_driver_change_restarts_output_and_keeps_page);
    RUN_TEST(test_saved_translation_reaches_oled_and_updates_live);
    RUN_TEST(test_disabled_enable_refresh_and_live_edit);
    RUN_TEST(test_missing_display_retries_and_recovers);
    RUN_TEST(test_line_errors_clear_stale_text_and_recover);
    RUN_TEST(test_hardware_change_and_clock_wrap);
    return UNITY_END();
}
