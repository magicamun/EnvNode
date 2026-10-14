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
#include "LocalizedDecimal.h"
#include "GeoPositionWebView.h"
#include "PressureReductionWebView.h"
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

void test_decimal_inputs_follow_locale_without_localizing_storage() {
    String canonical;
    TEST_ASSERT_TRUE(normalizeDecimalInput("-48,125", Locale::GermanGermany, canonical));
    TEST_ASSERT_EQUAL_STRING("-48.125", canonical.c_str());
    TEST_ASSERT_TRUE(normalizeDecimalInput(",5", Locale::GermanGermany, canonical));
    TEST_ASSERT_EQUAL_STRING("0.5", canonical.c_str());
    TEST_ASSERT_FALSE(normalizeDecimalInput("48.125", Locale::GermanGermany, canonical));
    TEST_ASSERT_EQUAL_STRING("", canonical.c_str());
    for (auto locale : {Locale::EnglishUnitedKingdom, Locale::EnglishUnitedStates}) {
        TEST_ASSERT_TRUE(normalizeDecimalInput("48.125", locale, canonical));
        TEST_ASSERT_EQUAL_STRING("48.125", canonical.c_str());
        TEST_ASSERT_FALSE(normalizeDecimalInput("48,125", locale, canonical));
    }
    for (const char* input : {"1.234,5", "1,2,3", "1 234,5", "nan", "1e3", "--1", ","})
        TEST_ASSERT_FALSE(normalizeDecimalInput(input, Locale::GermanGermany, canonical));
    GeoPosition position;
    TEST_ASSERT_TRUE(parseGeoPositionInput("48,5", "-11,25", "0", Locale::GermanGermany, position));
    TEST_ASSERT_EQUAL_FLOAT(48.5, position.latitude.value);
    TEST_ASSERT_EQUAL_FLOAT(-11.25, position.longitude.value);
    TEST_ASSERT_TRUE(position.altitude.configured); TEST_ASSERT_EQUAL_FLOAT(0, position.altitude.value);
    const auto record = encodeGeoPosition(position);
    TEST_ASSERT_NOT_NULL(strstr(record.c_str(), "48.500000000"));
    TEST_ASSERT_NULL(strchr(record.c_str(), ','));
    TEST_ASSERT_TRUE(decodeGeoPosition(record, position));
    TEST_ASSERT_TRUE(parseGeoPositionInput("", "", "", Locale::GermanGermany, position));
    TEST_ASSERT_FALSE(position.altitude.configured);
    TEST_ASSERT_FALSE(parseGeoPositionInput("90,1", "", "", Locale::GermanGermany, position));
    TEST_ASSERT_FALSE(parseGeoPositionInput("48.5", "", "", Locale::GermanGermany, position));
    TEST_ASSERT_TRUE(parseGeoPositionInput("48.5", "-11.25", "0", Locale::EnglishUnitedStates, position));
    TEST_ASSERT_EQUAL_STRING("48,5000000", formatDecimalInput(48.5, 7, Locale::GermanGermany).c_str());
    TEST_ASSERT_EQUAL_STRING("1234.500", formatDecimalInput(1234.5, 3, Locale::EnglishUnitedStates).c_str());
}
void test_location_and_temperature_age_views_use_configured_locale() {
    Configuration configuration;
    TEST_ASSERT_TRUE(parseGeoPosition("48.5", "11.25", "694.5", configuration.geoPosition));
    const String german = buildGeoPositionConfigurationHtml(configuration.geoPosition, Locale::GermanGermany);
    TEST_ASSERT_NOT_NULL(strstr(german.c_str(), "value='48,5000000'"));
    TEST_ASSERT_NOT_NULL(strstr(german.c_str(), "value='694,500'"));
    TEST_ASSERT_NOT_NULL(strstr(german.c_str(), "inputmode='decimal'"));
    TEST_ASSERT_NOT_NULL(strstr(german.c_str(), "lang='de-DE'"));
    const String english = buildGeoPositionConfigurationHtml(configuration.geoPosition, Locale::EnglishUnitedKingdom);
    TEST_ASSERT_NOT_NULL(strstr(english.c_str(), "value='48.5000000'"));
    const String age = buildPressureReductionConfigurationHtml(configuration, Locale::GermanGermany);
    TEST_ASSERT_NOT_NULL(strstr(age.c_str(), "value='60,000'"));
    uint32_t milliseconds = 0;
    TEST_ASSERT_TRUE(parseTemperatureAgeInput("1,234", Locale::GermanGermany, milliseconds));
    TEST_ASSERT_EQUAL(1234, milliseconds);
    TEST_ASSERT_TRUE(parseTemperatureAgeInput("0,001", Locale::GermanGermany, milliseconds));
    TEST_ASSERT_EQUAL(1, milliseconds);
    TEST_ASSERT_TRUE(parseTemperatureAgeInput("2147483,647", Locale::GermanGermany, milliseconds));
    TEST_ASSERT_EQUAL(INT32_MAX, milliseconds);
    TEST_ASSERT_FALSE(parseTemperatureAgeInput("2147483,648", Locale::GermanGermany, milliseconds));
    TEST_ASSERT_FALSE(parseTemperatureAgeInput("1.234", Locale::GermanGermany, milliseconds));
    TEST_ASSERT_TRUE(parseTemperatureAgeInput("1.234", Locale::EnglishUnitedStates, milliseconds));
    TEST_ASSERT_EQUAL(1234, milliseconds);
    TEST_ASSERT_FALSE(parseTemperatureAgeInput("1,234", Locale::EnglishUnitedStates, milliseconds));
    PressureReductionStatus status; status.hasTemperatureAge = true; status.temperatureAgeMs = 1234;
    TEST_ASSERT_NOT_NULL(strstr(buildPressureReductionDiagnosticHtml(status, Locale::GermanGermany).c_str(), "1,234 s"));
    TEST_ASSERT_NOT_NULL(strstr(buildPressureReductionDiagnosticHtml(status, Locale::EnglishUnitedStates).c_str(), "1.234 s"));
}
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_decimal_inputs_follow_locale_without_localizing_storage);
    RUN_TEST(test_location_and_temperature_age_views_use_configured_locale);
    RUN_TEST(test_locale_aliases_and_invalid_input);
    RUN_TEST(test_translation_and_english_fallback);
    RUN_TEST(test_navigation_keeps_routes_and_active_state);
    RUN_TEST(test_german_views_preserve_configuration_values);
    RUN_TEST(test_web_format_errors_leave_diagnostics_unchanged);
    RUN_TEST(test_log_interface_preserves_log_messages_and_protocol);
    return UNITY_END();
}
