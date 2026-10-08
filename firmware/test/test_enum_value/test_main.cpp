#include <unity.h>
#include "EnumValue.h"
using namespace EnvNode;
HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t length) { return length; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}

EnumValueConfiguration mode() {
    EnumValueConfiguration c;
    c.id = 1; c.name = "Wasserquelle"; c.defaultCode = "auto";
    c.options = {{"auto", "Automatik"}, {"cistern", "Zisterne"}, {"mains", "Hauswasser"}};
    return c;
}
void test_mode_changes_are_validated_and_revision_counts_changes() {
    EnumValue value(mode());
    TEST_ASSERT_NULL(value.current());
    TEST_ASSERT_TRUE(value.set("cistern") == EnumValueSetResult::NotStarted);
    TEST_ASSERT_TRUE(value.begin() == EnumValueStartResult::DefaultSelected);
    TEST_ASSERT_EQUAL_STRING("auto", value.current()->code.c_str());
    TEST_ASSERT_TRUE(value.set("cistern") == EnumValueSetResult::Changed);
    TEST_ASSERT_EQUAL_STRING("Zisterne", value.current()->label.c_str());
    TEST_ASSERT_TRUE(value.set("cistern") == EnumValueSetResult::Unchanged);
    TEST_ASSERT_TRUE(value.set("CISTERN") == EnumValueSetResult::UnknownOption);
    TEST_ASSERT_TRUE(value.set("") == EnumValueSetResult::UnknownOption);
    TEST_ASSERT_EQUAL_STRING("cistern", value.current()->code.c_str());
    TEST_ASSERT_EQUAL_UINT32(1, value.revision());
    TEST_ASSERT_TRUE(value.set("mains") == EnumValueSetResult::Changed);
    TEST_ASSERT_EQUAL_UINT32(2, value.revision());
}
void test_restart_policy_is_explicit() {
    EnumValue defaults(mode());
    TEST_ASSERT_TRUE(defaults.begin("mains") == EnumValueStartResult::DefaultSelected);
    TEST_ASSERT_EQUAL_STRING("auto", defaults.current()->code.c_str());
    auto c = mode(); c.restartPolicy = ValueRestartPolicy::RestoreLastValue;
    EnumValue restored(c);
    TEST_ASSERT_TRUE(restored.begin("mains") == EnumValueStartResult::Restored);
    TEST_ASSERT_EQUAL_STRING("mains", restored.current()->code.c_str());
    restored.set("cistern");
    TEST_ASSERT_TRUE(restored.begin("removed_option") == EnumValueStartResult::SavedValueRejected);
    TEST_ASSERT_EQUAL_STRING("auto", restored.current()->code.c_str());
    TEST_ASSERT_EQUAL_UINT32(0, restored.revision());
    TEST_ASSERT_TRUE(restored.begin() == EnumValueStartResult::DefaultSelected);
}
void test_invalid_definitions_cannot_be_used() {
    auto reject = [](const EnumValueConfiguration& c) {
        TEST_ASSERT_FALSE(validEnumValueConfiguration(c));
        EnumValue value(c);
        TEST_ASSERT_TRUE(value.begin() == EnumValueStartResult::InvalidConfiguration);
        TEST_ASSERT_NULL(value.current());
        TEST_ASSERT_TRUE(value.set("auto") == EnumValueSetResult::NotStarted);
    };
    auto c = mode(); c.id = 0; reject(c);
    c = mode(); c.name = ""; reject(c);
    c = mode(); c.defaultCode = "missing"; reject(c);
    c = mode(); c.options[1].code = "auto"; reject(c);
    c = mode(); c.options[1].code = "has space"; reject(c);
    c = mode(); c.options[1].label = "bad\nlabel"; reject(c);
    c = mode(); c.options.clear(); reject(c);
    c = mode(); c.options.resize(17); reject(c);
    c = mode(); c.restartPolicy = static_cast<ValueRestartPolicy>(99); reject(c);
}
void test_definition_is_owned() {
    auto c = mode(); EnumValue value(c);
    c.options.clear(); c.defaultCode = "changed";
    TEST_ASSERT_TRUE(value.begin() == EnumValueStartResult::DefaultSelected);
    TEST_ASSERT_TRUE(value.set("cistern") == EnumValueSetResult::Changed);
    TEST_ASSERT_EQUAL_STRING("Zisterne", value.current()->label.c_str());
}
int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_mode_changes_are_validated_and_revision_counts_changes);
    RUN_TEST(test_restart_policy_is_explicit);
    RUN_TEST(test_invalid_definitions_cannot_be_used);
    RUN_TEST(test_definition_is_owned);
    return UNITY_END();
}
