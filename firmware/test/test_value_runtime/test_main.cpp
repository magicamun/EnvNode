#include <unity.h>
#include <cstring>
#include "ConfigurationService.h"
#include "ValueRuntime.h"
#include "ValueWebView.h"
#include "ValuePropertyReader.h"
#include "DisplayPageFormatter.h"
#include "PropertyPreviewWebView.h"
using namespace EnvNode;
HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t n) { return n; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}
void setUp() { Preferences().clear(); Preferences::failStringWrites() = false; }
void tearDown() { Preferences::failStringWrites() = false; }
EnumValueConfiguration mode(ValueId id = 1) {
    EnumValueConfiguration c; c.id = id; c.name = "Wasserquelle";
    c.options = {{"auto", "Automatik"}, {"cistern", "Zisterne"}, {"mains", "Hauswasser"}};
    c.defaultCode = "auto"; return c;
}
void test_live_commands_restart_and_storage_failures() {
    ConfigurationService cfg; cfg.loadConfiguration(); ValueRuntime runtime(cfg); runtime.begin();
    TEST_ASSERT_TRUE(runtime.saveDefinition(mode(), true));
    TEST_ASSERT_TRUE(runtime.set(1, "cistern") == ValueCommandResult::Changed);
    TEST_ASSERT_EQUAL_STRING("cistern", runtime.find(1)->current()->code.c_str());
    TEST_ASSERT_TRUE(runtime.set(1, "other") == ValueCommandResult::UnknownOption);
    TEST_ASSERT_TRUE(runtime.set(2, "auto") == ValueCommandResult::UnknownValue);
    // Transient writes require no NVS write.
    Preferences::failStringWrites() = true;
    TEST_ASSERT_TRUE(runtime.set(1, "mains") == ValueCommandResult::Changed);
    Preferences::failStringWrites() = false;
    runtime.begin();
    TEST_ASSERT_EQUAL_STRING("auto", runtime.find(1)->current()->code.c_str());
    auto persistent = mode(); persistent.restartPolicy = ValueRestartPolicy::RestoreLastValue;
    TEST_ASSERT_TRUE(runtime.saveDefinition(persistent, false));
    TEST_ASSERT_TRUE(runtime.set(1, "cistern") == ValueCommandResult::Changed);
    Preferences::failStringWrites() = true;
    TEST_ASSERT_TRUE(runtime.set(1, "mains") == ValueCommandResult::StorageFailed);
    TEST_ASSERT_EQUAL_STRING("cistern", runtime.find(1)->current()->code.c_str());
    TEST_ASSERT_TRUE(runtime.set(1, "cistern") == ValueCommandResult::Unchanged);
    Preferences::failStringWrites() = false;
    ConfigurationService reboot; reboot.loadConfiguration(); ValueRuntime restored(reboot); restored.begin();
    TEST_ASSERT_EQUAL_STRING("cistern", restored.find(1)->current()->code.c_str());
}
void test_definition_changes_are_isolated_and_atomic() {
    ConfigurationService cfg; cfg.loadConfiguration(); ValueRuntime runtime(cfg); runtime.begin();
    TEST_ASSERT_TRUE(runtime.saveDefinition(mode(1), true));
    TEST_ASSERT_TRUE(runtime.saveDefinition(mode(2), true));
    runtime.set(2, "mains");
    TEST_ASSERT_FALSE(runtime.saveDefinition(mode(1), true));
    TEST_ASSERT_FALSE(runtime.saveDefinition(mode(3), false));
    auto edited = mode(1); edited.defaultCode = "cistern";
    Preferences::failStringWrites() = true;
    TEST_ASSERT_FALSE(runtime.saveDefinition(edited, false));
    TEST_ASSERT_FALSE(runtime.remove(1));
    TEST_ASSERT_EQUAL_STRING("auto", runtime.find(1)->current()->code.c_str());
    Preferences::failStringWrites() = false;
    TEST_ASSERT_TRUE(runtime.saveDefinition(edited, false));
    TEST_ASSERT_EQUAL_STRING("cistern", runtime.find(1)->current()->code.c_str());
    TEST_ASSERT_EQUAL_STRING("mains", runtime.find(2)->current()->code.c_str());
    TEST_ASSERT_TRUE(runtime.remove(1)); TEST_ASSERT_NULL(runtime.find(1));
    TEST_ASSERT_FALSE(runtime.remove(1));
    TEST_ASSERT_EQUAL_STRING("mains", runtime.find(2)->current()->code.c_str());
}
void test_capacity_rejects_extra_value_without_changing_runtime() {
    ConfigurationService cfg; cfg.loadConfiguration(); ValueRuntime runtime(cfg); runtime.begin();
    for (unsigned int i=1; i<=MaxEnumValueCount; ++i) TEST_ASSERT_TRUE(runtime.saveDefinition(mode(i), true));
    TEST_ASSERT_FALSE(runtime.saveDefinition(mode(9), true));
    TEST_ASSERT_EQUAL_UINT32(MaxEnumValueCount, runtime.values().size());
    TEST_ASSERT_NULL(strstr(buildValuesHtml(runtime).c_str(), "href='/values/edit'"));
}
void test_web_view_escapes_user_text_and_exposes_post_controls() {
    ConfigurationService cfg; cfg.loadConfiguration(); ValueRuntime runtime(cfg); runtime.begin();
    TEST_ASSERT_NOT_NULL(strstr(buildValuesHtml(runtime).c_str(), "Water source example"));
    auto c = mode(); c.name = "<script>bad</script>"; c.options[1].label = "A & B";
    TEST_ASSERT_TRUE(runtime.saveDefinition(c, true)); runtime.set(1, "cistern");
    const String html = buildValuesHtml(runtime);
    TEST_ASSERT_NULL(strstr(html.c_str(), "<script>bad"));
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "&lt;script&gt;"));
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "A &amp; B"));
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "method='post' action='/values/set'"));
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "value='cistern' selected"));
    const String editor = buildValueEditorHtml(c, false, "Invalid <value>");
    TEST_ASSERT_NOT_NULL(strstr(editor.c_str(), "Invalid &lt;value&gt;"));
    TEST_ASSERT_NOT_NULL(strstr(editor.c_str(), "name='code15'"));
    TEST_ASSERT_NOT_NULL(strstr(editor.c_str(), "action='/values/delete'"));
    TEST_ASSERT_NULL(strstr(buildValueEditorHtml(c, true).c_str(), "action='/values/delete'"));
}
void test_id_parser_rejects_ambiguous_or_out_of_range_ids() {
    ValueId id = 10;
    TEST_ASSERT_TRUE(parseValueId("65535", id)); TEST_ASSERT_EQUAL_UINT32(65535,id);
    for (const auto* text : {"", "0", "-1", "1x", "65536", " 1", "999999999"}) TEST_ASSERT_FALSE(parseValueId(text,id));
}
void test_value_display_format_translation_and_live_changes() {
    ConfigurationService cfg; cfg.loadConfiguration(); ValueRuntime runtime(cfg); runtime.begin();
    ValuePropertyReader reader(runtime);
    TEST_ASSERT_TRUE(runtime.saveDefinition(mode(), true));
    DisplayConfiguration page; page.formats[0] = "Modus: %s"; page.sources[0][0] = "value/1/state";
    TEST_ASSERT_TRUE(cfg.setDisplayConfiguration(page));
    auto line = formatDisplayLine(reader, page, 0);
    TEST_ASSERT_TRUE(line.value.status == PropertyFormatStatus::Formatted);
    TEST_ASSERT_EQUAL_STRING("Modus: Automatik", line.value.text);
    runtime.set(1, "cistern");
    TEST_ASSERT_EQUAL_STRING("Modus: Zisterne", formatDisplayLine(reader, page, 0).value.text);
    DisplayEnumTranslation label; label.line = 0; label.source = 0; label.code = "cistern"; label.text = "Regenwasser";
    page.enumTranslations.push_back(label);
    TEST_ASSERT_EQUAL_STRING("Modus: Regenwasser", formatDisplayLine(reader, page, 0).value.text);
    runtime.set(1, "mains");
    TEST_ASSERT_EQUAL_STRING("Modus: Hauswasser", formatDisplayLine(reader, page, 0).value.text);
    runtime.remove(1);
    TEST_ASSERT_TRUE(formatDisplayLine(reader, page, 0).value.status == PropertyFormatStatus::UnknownReference);
}
void test_dynamic_enum_snapshots_and_descriptions_own_their_text() {
    ConfigurationService cfg; cfg.loadConfiguration(); ValueRuntime runtime(cfg); runtime.begin();
    ValuePropertyReader reader(runtime); runtime.saveDefinition(mode(), true);
    const PropertyReference ref(PropertyComponentKind::Value, 1, "state");
    PropertyDescription description; TEST_ASSERT_TRUE(reader.describe(ref, description));
    PropertySnapshot snapshot; TEST_ASSERT_TRUE(reader.read(ref, snapshot) == PropertyReadResult::Available);
    PropertySnapshot copy = snapshot; snapshot = PropertySnapshot{};
    const String option = buildPropertySourceOption(ref, description, "Mode", "");
    TEST_ASSERT_NOT_NULL(strstr(option.c_str(), "value/1/state"));
    TEST_ASSERT_NOT_NULL(strstr(option.c_str(), "data-enum="));
    runtime.remove(1);
    const PropertyEnumOption* value = nullptr;
    TEST_ASSERT_TRUE(copy.value.tryGetEnumeration(value));
    TEST_ASSERT_EQUAL_STRING("auto", value->stableCode);
    TEST_ASSERT_EQUAL_STRING("Automatik", value->displayText);
    TEST_ASSERT_EQUAL_STRING("cistern", description.enumOptions[1].stableCode);
    TEST_ASSERT_TRUE(copy.hasRevision);
    TEST_ASSERT_FALSE(reader.describe(ref, description));
    TEST_ASSERT_EQUAL_UINT32(0, description.enumOptionCount);
    TEST_ASSERT_TRUE(reader.read(ref, snapshot) == PropertyReadResult::UnknownReference);
    TEST_ASSERT_FALSE(snapshot.valid);
}
int main(int,char**) {
    UNITY_BEGIN();
    RUN_TEST(test_value_display_format_translation_and_live_changes);
    RUN_TEST(test_dynamic_enum_snapshots_and_descriptions_own_their_text);
    RUN_TEST(test_live_commands_restart_and_storage_failures);
    RUN_TEST(test_definition_changes_are_isolated_and_atomic);
    RUN_TEST(test_capacity_rejects_extra_value_without_changing_runtime);
    RUN_TEST(test_web_view_escapes_user_text_and_exposes_post_controls);
    RUN_TEST(test_id_parser_rejects_ambiguous_or_out_of_range_ids);
    return UNITY_END();
}
