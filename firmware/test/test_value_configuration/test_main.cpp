#include <unity.h>
#include <string>
#include "ConfigurationService.h"
using namespace EnvNode;
HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t length) { return length; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}
void setUp() { Preferences().clear(); Preferences::failStringWrites() = false; }
void tearDown() { Preferences::failStringWrites() = false; }
EnumValueConfiguration mode(ValueId id = 1) {
    EnumValueConfiguration c;
    c.id = id; c.name = "Wasserquelle"; c.defaultCode = "auto";
    c.options = {{"auto", "Automatik"}, {"cistern", "Zisterne"}, {"mains", "Hauswasser"}};
    c.restartPolicy = ValueRestartPolicy::RestoreLastValue;
    return c;
}
void test_definition_checkpoint_reload_and_reset() {
    ConfigurationService service; service.loadConfiguration();
    TEST_ASSERT_TRUE(service.getConfiguration().values.empty());
    TEST_ASSERT_TRUE(service.setDeviceName("Keep me"));
    TEST_ASSERT_TRUE(service.setEnumValueDefinitions({mode()}));
    TEST_ASSERT_TRUE(service.saveEnumValueCode(1, "cistern"));
    ConfigurationService loaded; loaded.loadConfiguration();
    TEST_ASSERT_EQUAL_UINT32(1, loaded.getConfiguration().values.size());
    const auto& stored = loaded.getConfiguration().values[0];
    EnumValue value(stored.definition);
    TEST_ASSERT_TRUE(value.begin(stored.savedCode) == EnumValueStartResult::Restored);
    TEST_ASSERT_EQUAL_STRING("cistern", value.current()->code.c_str());
    TEST_ASSERT_EQUAL_STRING("Keep me", loaded.getConfiguration().device.name.c_str());
    TEST_ASSERT_TRUE(loaded.resetToDefaults());
    service.loadConfiguration();
    TEST_ASSERT_TRUE(service.getConfiguration().values.empty());
}
void test_failed_writes_and_invalid_changes_are_atomic() {
    ConfigurationService service; service.loadConfiguration();
    TEST_ASSERT_TRUE(service.setEnumValueDefinitions({mode()}));
    TEST_ASSERT_TRUE(service.saveEnumValueCode(1, "cistern"));
    const String before = Preferences::storedString("enum_values1");
    TEST_ASSERT_FALSE(service.saveEnumValueCode(2, "mains"));
    TEST_ASSERT_FALSE(service.saveEnumValueCode(1, "invalid"));
    TEST_ASSERT_FALSE(service.saveEnumValueCode(1, ""));
    TEST_ASSERT_FALSE(service.setEnumValueDefinitions({mode(), mode()}));
    Preferences::failStringWrites() = true;
    TEST_ASSERT_TRUE(service.saveEnumValueCode(1, "cistern"));
    TEST_ASSERT_FALSE(service.saveEnumValueCode(1, "mains"));
    TEST_ASSERT_FALSE(service.setEnumValueDefinitions({}));
    TEST_ASSERT_EQUAL_STRING(before.c_str(), Preferences::storedString("enum_values1").c_str());
    TEST_ASSERT_EQUAL_STRING("cistern", service.getConfiguration().values[0].savedCode.c_str());
}
void test_editing_and_reordering_preserves_only_compatible_checkpoints() {
    ConfigurationService service; service.loadConfiguration();
    TEST_ASSERT_TRUE(service.setEnumValueDefinitions({mode(1), mode(2)}));
    TEST_ASSERT_TRUE(service.saveEnumValueCode(1, "cistern"));
    TEST_ASSERT_TRUE(service.setEnumValueDefinitions({mode(2), mode(1)}));
    TEST_ASSERT_EQUAL_STRING("cistern", service.getConfiguration().values[1].savedCode.c_str());
    auto changed = mode(1); changed.options.erase(changed.options.begin() + 1);
    TEST_ASSERT_TRUE(service.setEnumValueDefinitions({changed}));
    TEST_ASSERT_EQUAL_STRING("", service.getConfiguration().values[0].savedCode.c_str());
    TEST_ASSERT_TRUE(service.saveEnumValueCode(1, "mains"));
    changed.restartPolicy = ValueRestartPolicy::DefaultOnRestart;
    TEST_ASSERT_TRUE(service.setEnumValueDefinitions({changed}));
    TEST_ASSERT_FALSE(service.saveEnumValueCode(1, "mains"));
    ConfigurationService loaded; loaded.loadConfiguration();
    const auto& stored = loaded.getConfiguration().values[0];
    EnumValue value(stored.definition); value.begin(stored.savedCode);
    TEST_ASSERT_EQUAL_STRING("auto", value.current()->code.c_str());
    TEST_ASSERT_TRUE(service.setEnumValueDefinitions({}));
    TEST_ASSERT_TRUE(service.setEnumValueDefinitions({mode(1)}));
    TEST_ASSERT_EQUAL_STRING("", service.getConfiguration().values[0].savedCode.c_str());
}
void test_codec_rejects_malformed_records_without_partial_output() {
    StoredEnumValue item; item.definition = mode(); item.savedCode = "mains";
    ValueConfiguration input = {item}, result;
    String encoded;
    TEST_ASSERT_TRUE(encodeValueConfiguration(input, encoded));
    TEST_ASSERT_TRUE(decodeValueConfiguration(encoded, result));
    TEST_ASSERT_EQUAL_STRING("mains", result[0].savedCode.c_str());
    const String bad[] = {String("2\n0\n"), String("1\n9\n"), String("1\n-1\n"), encoded + "extra", String(std::string(encoded.c_str()).substr(0, encoded.length()-1).c_str())};
    for (const auto& record : bad) {
        TEST_ASSERT_FALSE(decodeValueConfiguration(record, result));
        TEST_ASSERT_EQUAL_STRING("mains", result[0].savedCode.c_str());
    }
    input.resize(9, item);
    TEST_ASSERT_FALSE(encodeValueConfiguration(input, encoded));
}
void test_corrupt_storage_isolated_from_other_configuration() {
    ConfigurationService service; service.loadConfiguration();
    TEST_ASSERT_TRUE(service.setDeviceName("Keep me"));
    Preferences prefs;
    const char bad[] = "1\n1\ntruncated";
    prefs.putBytes("enum_values1", bad, sizeof(bad)-1);
    service.loadConfiguration();
    TEST_ASSERT_TRUE(service.getConfiguration().values.empty());
    TEST_ASSERT_EQUAL_STRING("Keep me", service.getConfiguration().device.name.c_str());
}
void test_full_capacity_roundtrip() {
    std::vector<EnumValueConfiguration> definitions;
    for (unsigned int i = 1; i <= MaxEnumValueCount; ++i) {
        auto c = mode(i);
        c.name = std::string(64, 'N').c_str();
        c.options.clear();
        for (unsigned int j = 0; j < 16; ++j) {
            EnumValueOption option;
            option.code = (std::string(30, 'a') + (j < 10 ? "0" : "1") + std::to_string(j % 10)).c_str();
            option.label = std::string(64, 'L').c_str();
            c.options.push_back(option);
        }
        c.defaultCode = c.options[0].code;
        definitions.push_back(c);
    }
    ConfigurationService service; service.loadConfiguration();
    TEST_ASSERT_TRUE(service.setEnumValueDefinitions(definitions));
    TEST_ASSERT_TRUE(Preferences::storedString("enum_values1").length() > 4000);
    ConfigurationService loaded; loaded.loadConfiguration();
    TEST_ASSERT_EQUAL_UINT32(MaxEnumValueCount, loaded.getConfiguration().values.size());
    TEST_ASSERT_EQUAL_UINT32(16, loaded.getConfiguration().values.back().definition.options.size());
}
int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_full_capacity_roundtrip);
    RUN_TEST(test_definition_checkpoint_reload_and_reset);
    RUN_TEST(test_failed_writes_and_invalid_changes_are_atomic);
    RUN_TEST(test_editing_and_reordering_preserves_only_compatible_checkpoints);
    RUN_TEST(test_codec_rejects_malformed_records_without_partial_output);
    RUN_TEST(test_corrupt_storage_isolated_from_other_configuration);
    return UNITY_END();
}
