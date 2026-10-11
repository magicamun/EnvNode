// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
// See the repository root LICENSE.
#include <unity.h>
#include <cstring>
#include "WebLocalization.h"
#include "WebNavigation.h"
#include "WebPresentationText.h"
#include "ValueWebView.h"
#include "PropertyPreviewWebView.h"
#include "LogWebView.h"
#include "RecentLogStore.h"
using namespace EnvNode;
HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t length) { return length; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}
void setUp() {}
void tearDown() {}
void test_locale_aliases_and_invalid_input() {
    Locale locale = Locale::EnglishUnitedKingdom;
    TEST_ASSERT_TRUE(parseLocaleKey("de_de", locale));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Locale::GermanGermany), static_cast<int>(locale));
    TEST_ASSERT_EQUAL_STRING("de-DE", localeKey(locale));
    TEST_ASSERT_TRUE(parseLocaleKey("en_US", locale));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Locale::EnglishUnitedStates), static_cast<int>(locale));
    TEST_ASSERT_FALSE(parseLocaleKey("de", locale));
    TEST_ASSERT_FALSE(parseLocaleKey(nullptr, locale));
    TEST_ASSERT_FALSE(parseLocaleKey("de-DE-extra", locale));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Locale::EnglishUnitedStates), static_cast<int>(locale));
}
void test_translation_and_english_fallback() {
    TEST_ASSERT_EQUAL_STRING("Sprache & Zeit", webText(Locale::GermanGermany, TextId::LocaleTime));
    TEST_ASSERT_EQUAL_STRING("Sensors", webText(Locale::EnglishUnitedKingdom, TextId::Sensors));
    TEST_ASSERT_EQUAL_STRING("Sensors", webText(Locale::EnglishUnitedStates, TextId::Sensors));
    TEST_ASSERT_EQUAL_STRING("Sensors", webText(static_cast<Locale>(255), TextId::Sensors));
    TEST_ASSERT_EQUAL_STRING("Status", webText(Locale::GermanGermany, TextId::Status));
    TEST_ASSERT_EQUAL_STRING("", webText(Locale::GermanGermany, static_cast<TextId>(65535)));
    for (unsigned i = 0; i < static_cast<unsigned>(TextId::Count); ++i) {
        TEST_ASSERT_TRUE(webText(Locale::EnglishUnitedKingdom, static_cast<TextId>(i))[0] != '\0');
    }
    TEST_ASSERT_EQUAL_STRING("de-DE", webLanguageTag(Locale::GermanGermany));
    TEST_ASSERT_EQUAL_STRING("en", webLanguageTag(static_cast<Locale>(255)));
}
void test_navigation_keeps_routes_and_active_state() {
    const String german = buildWebNavigationHtml("/time", Locale::GermanGermany);
    TEST_ASSERT_NOT_NULL(strstr(german.c_str(), "href='/time' class='active'>Sprache & Zeit"));
    TEST_ASSERT_NOT_NULL(strstr(german.c_str(), "href='/sensors' class=''>Sensoren"));
    TEST_ASSERT_NOT_NULL(strstr(german.c_str(), "href='/configuration' class=''>Konfiguration"));
    TEST_ASSERT_NOT_NULL(strstr(german.c_str(), "href='/hardware' class=''>Hardware"));
    const String hardware = buildWebNavigationHtml("/hardware");
    TEST_ASSERT_NOT_NULL(strstr(hardware.c_str(), "href='/hardware' class='active'>Hardware"));
    const String english = buildWebNavigationHtml("/time");
    TEST_ASSERT_NOT_NULL(strstr(english.c_str(), "href='/time' class='active'>Locale & Time"));
}
void test_german_views_preserve_configuration_values() {
    EnumValueConfiguration definition;
    definition.id = 3;
    definition.name = "Save";
    definition.defaultCode = "on";
    definition.options.push_back({"on", "Custom label"});
    const String html = buildValueEditorHtml(definition, true, nullptr, Locale::GermanGermany);
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "Wert hinzufügen"));
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "Definition speichern"));
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "value='Save'"));
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "value='Custom label'"));
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "value='on'"));
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "action='/values/save'"));
    const String english = buildValueEditorHtml(definition, true);
    TEST_ASSERT_NOT_NULL(strstr(english.c_str(), "Save definition"));
    const String displays = buildDisplayOutputsHtml(DisplayConfiguration{}, Locale::GermanGermany);
    TEST_ASSERT_NOT_NULL(strstr(displays.c_str(), "Auf dem Gerät speichern"));
    TEST_ASSERT_NOT_NULL(strstr(displays.c_str(), "aria-label='Anzeigekonfiguration'"));
}
void test_web_format_errors_leave_diagnostics_unchanged() {
    TEST_ASSERT_EQUAL_STRING("Relative Luftfeuchtigkeit", webMeasurementName(Locale::GermanGermany, MeasurementType::RelativeHumidity));
    TEST_ASSERT_EQUAL_STRING("good", measurementQualityStableId(MeasurementQuality::Good));
    TEST_ASSERT_EQUAL_STRING("Gut", webQualityName(Locale::GermanGermany, MeasurementQuality::Good));
    TEST_ASSERT_EQUAL_STRING("No value available yet.", propertyFormatError(PropertyFormatStatus::NoValue));
    TEST_ASSERT_EQUAL_STRING("Noch kein Wert verfügbar.", webFormatError(Locale::GermanGermany, PropertyFormatStatus::NoValue));
    DisplayLineResult result;
    result.issue = DisplayLineIssue::SourceGap;
    result.error = "Fill sources in order without gaps.";
    TEST_ASSERT_EQUAL_STRING("Quellen in Reihenfolge ohne Lücken angeben.", webDisplayLineError(Locale::GermanGermany, result));
}
void test_log_interface_preserves_log_messages_and_protocol() {
    RecentLogStore store;
    LogEntry entry;
    entry.sequence = 1;
    entry.level = LogLevel::Error;
    std::strcpy(entry.message, "Save failed: driver unavailable");
    store.append(entry);
    const String html = buildRecentLogHtml(store, Locale::GermanGermany);
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "Automatisch aktualisieren"));
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "Save failed: driver unavailable"));
    const String json = buildRecentLogJson(store);
    TEST_ASSERT_NOT_NULL(strstr(json.c_str(), "\"level\":\"ERROR\""));
    TEST_ASSERT_NOT_NULL(strstr(json.c_str(), "Save failed: driver unavailable"));
    const String script = buildWebUiScript(Locale::GermanGermany);
    TEST_ASSERT_NOT_NULL(strstr(script.c_str(), "\"SelectASource\":\"Eine Quelle auswählen\""));
}
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_locale_aliases_and_invalid_input);
    RUN_TEST(test_translation_and_english_fallback);
    RUN_TEST(test_navigation_keeps_routes_and_active_state);
    RUN_TEST(test_german_views_preserve_configuration_values);
    RUN_TEST(test_web_format_errors_leave_diagnostics_unchanged);
    RUN_TEST(test_log_interface_preserves_log_messages_and_protocol);
    return UNITY_END();
}
