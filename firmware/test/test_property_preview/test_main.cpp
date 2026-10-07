#include <Arduino.h>
#include <unity.h>
#include <cstring>
#include <limits>
#include <string>
#include "PropertyTextFormatter.h"
#include "PropertyPreviewWebView.h"

using namespace EnvNode;

HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t length) { return length; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}

const PropertyReference Source(PropertyComponentKind::Sensor, 3, "temperature");

class Reader : public IPropertyReader {
public:
    PropertyDescription description;
    PropertySnapshot snapshot;
    bool known = true;
    PropertyReadResult status = PropertyReadResult::Available;
    mutable unsigned reads = 0;
    Reader() {
        description.stableKey = "temperature";
        description.valueKind = PropertyValueKind::FloatingPoint;
        description.canonicalUnit = PresentationUnit::DegreeCelsius;
        snapshot.valid = true;
        snapshot.value = MeasurementValue::floatingPoint(21.25F);
    }
    bool describe(const PropertyReference&, PropertyDescription& result) const override {
        result = description;
        return known;
    }
    PropertyReadResult read(const PropertyReference&, PropertySnapshot& result) const override {
        ++reads;
        result = snapshot;
        return status;
    }
};

void expectText(Reader& reader, const char* format, const char* expected) {
    const auto result = formatPropertyText(reader, Source, format);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyFormatStatus::Formatted), static_cast<int>(result.status));
    TEST_ASSERT_EQUAL_STRING(expected, result.text);
}
void expectError(Reader& reader, const char* format, PropertyFormatStatus status) {
    const auto result = formatPropertyText(reader, Source, format);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(status), static_cast<int>(result.status));
    TEST_ASSERT_EQUAL_STRING("", result.text);
    TEST_ASSERT_TRUE(strlen(propertyFormatError(status)) > 0);
}

void test_float_width_precision_alignment_and_literal_percent() {
    Reader reader;
    expectText(reader, "Temperature: %.1f C", "Temperature: 21.2 C");
    expectText(reader, "[%8.2f]", "[   21.25]");
    expectText(reader, "[%-8.2f]", "[21.25   ]");
    expectText(reader, "%08.2f %%", "00021.25 %");
    expectText(reader, "%%%f%%", "%21.250000%");
    reader.snapshot.value = MeasurementValue::floatingPoint(-3.5F);
    expectText(reader, "%07.1f", "-0003.5");
    expectText(reader, "%-07.1f", "-3.5   ");
}

void test_integer_and_boolean_text() {
    Reader reader;
    reader.description.valueKind = PropertyValueKind::UnsignedInteger;
    reader.snapshot.value = MeasurementValue::unsignedInteger(UINT32_MAX);
    expectText(reader, "%u", "4294967295");
    reader.snapshot.value = MeasurementValue::unsignedInteger(42);
    expectText(reader, "%05u", "00042");
    reader.description.valueKind = PropertyValueKind::Boolean;
    reader.description.trueText = "On";
    reader.description.falseText = "Off";
    reader.snapshot.value = MeasurementValue::boolean(true);
    expectText(reader, "Valve: %-5s!", "Valve: On   !");
    reader.snapshot.value = MeasurementValue::boolean(false);
    expectText(reader, "Valve: %5s", "Valve:   Off");
}

void test_enum_uses_display_text_and_never_interprets_it_as_format() {
    static const PropertyEnumOption option{"on_threshold", "On threshold: 100% %n"};
    Reader reader;
    reader.description.valueKind = PropertyValueKind::Enumeration;
    reader.snapshot.value = PropertyValue::enumeration(option);
    expectText(reader, "Reason: %s", "Reason: On threshold: 100% %n");
}

void test_unsupported_formats_are_rejected_before_reading() {
    Reader reader;
    const char* invalid[] = {nullptr, "", "No placeholder", "%%", "%", "%n", "%p", "%d",
        "%lf", "%*f", "%.*f", "%1$f", "%+f", "%#f", "%65f", "%.7f", "%.f",
        "%.2s", "%0s", "%--f", "%00f", "%f %f", "%s %u", "%f\n", "%f\t", "%f\r"};
    for (const char* format : invalid) expectError(reader, format, PropertyFormatStatus::InvalidFormat);
    TEST_ASSERT_EQUAL_UINT32(0, reader.reads);
}

void test_type_mismatch_and_invalid_values() {
    Reader reader;
    expectError(reader, "%s", PropertyFormatStatus::TypeMismatch);
    expectError(reader, "%u", PropertyFormatStatus::TypeMismatch);
    TEST_ASSERT_EQUAL_UINT32(0, reader.reads);
    reader.snapshot.valid = false;
    expectError(reader, "%f", PropertyFormatStatus::InvalidValue);
    reader.snapshot.valid = true;
    reader.snapshot.value = MeasurementValue::boolean(true);
    expectError(reader, "%f", PropertyFormatStatus::InvalidValue);
    reader.snapshot.value = MeasurementValue::floatingPoint(std::numeric_limits<float>::quiet_NaN());
    expectError(reader, "%f", PropertyFormatStatus::InvalidValue);
    reader.snapshot.value = MeasurementValue::floatingPoint(std::numeric_limits<float>::infinity());
    expectError(reader, "%f", PropertyFormatStatus::InvalidValue);
}

void test_unknown_and_missing_sources_are_distinct() {
    Reader reader;
    reader.known = false;
    expectError(reader, "%f", PropertyFormatStatus::UnknownReference);
    reader.known = true;
    reader.status = PropertyReadResult::NoValue;
    expectError(reader, "%f", PropertyFormatStatus::NoValue);
    reader.status = PropertyReadResult::UnknownReference;
    expectError(reader, "%f", PropertyFormatStatus::UnknownReference);
}

void test_bounded_formats_and_output_never_return_partial_text() {
    Reader reader;
    const std::string longFormat = std::string(127, 'x') + "%f";
    expectError(reader, longFormat.c_str(), PropertyFormatStatus::FormatTooLong);
    reader.description.valueKind = PropertyValueKind::Boolean;
    reader.description.trueText = "123456";
    reader.snapshot.value = MeasurementValue::boolean(true);
    const std::string exact = std::string(122, 'x') + "%s";
    const auto result = formatPropertyText(reader, Source, exact.c_str());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyFormatStatus::Formatted), static_cast<int>(result.status));
    TEST_ASSERT_EQUAL_UINT32(128, strlen(result.text));
    const std::string overflow = std::string(123, 'x') + "%s";
    expectError(reader, overflow.c_str(), PropertyFormatStatus::OutputTooLong);
    reader.description.trueText = "bad\ntext";
    expectError(reader, "%s", PropertyFormatStatus::InvalidValue);
    const std::string longText(129, 'z');
    reader.description.trueText = longText.c_str();
    expectError(reader, "%s", PropertyFormatStatus::OutputTooLong);
}

void test_source_parser_requires_complete_bounded_reference() {
    PropertySourceInput parsed;
    TEST_ASSERT_TRUE(parsePropertySource("controller/2/reason", parsed));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyComponentKind::Controller), static_cast<int>(parsed.kind));
    TEST_ASSERT_EQUAL_UINT32(2, parsed.id);
    TEST_ASSERT_EQUAL_STRING("reason", parsed.key);
    TEST_ASSERT_TRUE(parsePropertySource("actuator/65535/state", parsed));
    TEST_ASSERT_TRUE(parsePropertySource("sensor/3/temperature", parsed));
    const char* invalid[] = {nullptr, "", "sensor", "sensor/", "sensor/1", "sensor/0/temperature",
        "sensor/-1/temperature", "sensor/65536/temperature", "sensor/9999999999999999/temperature",
        "sensor/1x/temperature", "sensor/1/", "sensor/1/a/b", "sensor/1/<script>", "Sensor/1/temperature"};
    for (const char* input : invalid) {
        TEST_ASSERT_FALSE(parsePropertySource(input, parsed));
        TEST_ASSERT_EQUAL_UINT32(0, parsed.id);
        TEST_ASSERT_EQUAL_STRING("", parsed.key);
    }
    const std::string tooLong = "sensor/1/" + std::string(100, 'x');
    TEST_ASSERT_FALSE(parsePropertySource(tooLong.c_str(), parsed));
}

void test_web_preview_keeps_selection_and_escapes_all_user_text() {
    Reader reader;
    const String source("sensor/3/temperature");
    const String options = buildPropertySourceOption(Source, reader.description, "<Tank>", source);
    TEST_ASSERT_NOT_NULL(strstr(options.c_str(), "selected"));
    TEST_ASSERT_NOT_NULL(strstr(options.c_str(), "&lt;Tank&gt;"));
    String html = buildPropertyPreviewHtml(reader, options, source, "<b title='x'>%.2f</b>", true, true);
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "&lt;b title=&#39;x&#39;&gt;21.25&lt;/b&gt;"));
    TEST_ASSERT_NULL(strstr(html.c_str(), "<b title="));
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "method='get'"));
    reader.snapshot.value = MeasurementValue::floatingPoint(22.5F);
    html = buildPropertyPreviewHtml(reader, options, source, "Value %.1f", true, true);
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "Value 22.5"));
    html = buildPropertyPreviewHtml(reader, options, source, "%s", true, true);
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "Format does not match"));
    TEST_ASSERT_NULL(strstr(html.c_str(), "<pre"));
    reader.known = false;
    html = buildPropertyPreviewHtml(reader, "", source, "%f", true, false);
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "Selected source unavailable"));
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "Source is unknown"));
}

void test_web_does_not_read_before_submission_and_handles_empty_inventory() {
    Reader reader;
    const String html = buildPropertyPreviewHtml(reader, "", "", "", false, false);
    TEST_ASSERT_EQUAL_UINT32(0, reader.reads);
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "No supported sources"));
    TEST_ASSERT_NULL(strstr(html.c_str(), "<pre"));
}

class OrderedReader : public Reader {
public:
    bool describe(const PropertyReference& ref, PropertyDescription& result) const override {
        if (ref.componentId == 99) return false;
        result = description;
        if (ref.componentId == 3) result.valueKind = PropertyValueKind::Boolean;
        if (ref.componentId == 4) result.valueKind = PropertyValueKind::Enumeration;
        return true;
    }
    PropertyReadResult read(const PropertyReference& ref, PropertySnapshot& result) const override {
        ++reads;
        result = snapshot;
        if (ref.componentId == 2) result.value = MeasurementValue::floatingPoint(52);
        if (ref.componentId == 3) result.value = MeasurementValue::boolean(true);
        if (ref.componentId == 4) {
            static const PropertyEnumOption option{"hold", "Hold"};
            result.value = PropertyValue::enumeration(option);
        }
        return status;
    }
};

void test_multiple_ordered_sources_and_mixed_types() {
    OrderedReader reader;
    const PropertyReference refs[] = {{PropertyComponentKind::Sensor, 1, "value"},
        {PropertyComponentKind::Sensor, 2, "value"}, {PropertyComponentKind::Actuator, 3, "state"},
        {PropertyComponentKind::Controller, 4, "reason"}};
    auto result = formatPropertyText(reader, refs, 2, "Level: %.0f l %.0f%%");
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyFormatStatus::Formatted), static_cast<int>(result.status));
    TEST_ASSERT_EQUAL_STRING("Level: 21 l 52%", result.text);
    result = formatPropertyText(reader, refs, 4, "%.2f / %.0f / %s / %s");
    TEST_ASSERT_EQUAL_STRING("21.25 / 52 / True / Hold", result.text);
    const PropertyReference reverse[] = {refs[1], refs[0]};
    result = formatPropertyText(reader, reverse, 2, "%.0f %.2f");
    TEST_ASSERT_EQUAL_STRING("52 21.25", result.text);
}

void test_literal_and_empty_lines_require_no_sources() {
    Reader reader;
    auto result = formatPropertyText(reader, nullptr, 0, "RainControl 100%%");
    TEST_ASSERT_EQUAL_STRING("RainControl 100%", result.text);
    result = formatPropertyText(reader, nullptr, 0, "");
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyFormatStatus::Formatted), static_cast<int>(result.status));
    TEST_ASSERT_EQUAL_STRING("", result.text);
    result = formatPropertyText(reader, nullptr, 0, "%f");
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyFormatStatus::SourceCountMismatch), static_cast<int>(result.status));
    result = formatPropertyText(reader, &Source, 1, "Literal");
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyFormatStatus::SourceCountMismatch), static_cast<int>(result.status));
    TEST_ASSERT_EQUAL_UINT32(0, reader.reads);
}

void test_multi_source_errors_clear_whole_line_and_bound_total_output() {
    OrderedReader reader;
    PropertyReference refs[] = {{PropertyComponentKind::Sensor, 1, "value"},
        {PropertyComponentKind::Sensor, 99, "missing"}};
    auto result = formatPropertyText(reader, refs, 2, "Value %.1f missing %.1f");
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyFormatStatus::UnknownReference), static_cast<int>(result.status));
    TEST_ASSERT_EQUAL_STRING("", result.text);
    refs[1] = {PropertyComponentKind::Actuator, 3, "state"};
    result = formatPropertyText(reader, refs, 2, "%f %f");
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyFormatStatus::TypeMismatch), static_cast<int>(result.status));
    TEST_ASSERT_EQUAL_STRING("", result.text);
    refs[1] = refs[0];
    result = formatPropertyText(reader, refs, 2, "%64f%64fX");
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyFormatStatus::OutputTooLong), static_cast<int>(result.status));
    TEST_ASSERT_EQUAL_STRING("", result.text);
    result = formatPropertyText(reader, refs, 2, "%f%f%f%f%f");
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyFormatStatus::InvalidFormat), static_cast<int>(result.status));
}

void test_six_line_page_preserves_layout_and_isolates_row_errors() {
    OrderedReader reader;
    PropertyPreviewPage page;
    page.formats[0] = "RainControl";
    page.formats[1] = "Level: %.0f l %.0f%%";
    page.sources[1][0] = "sensor/1/value";
    page.sources[1][1] = "sensor/2/value";
    page.formats[2] = "%s";
    page.sources[2][0] = "actuator/3/state";
    page.formats[3] = "Reason: %s";
    page.sources[3][0] = "controller/4/reason";
    page.formats[5] = "<End>";
    String html = buildPropertyPagePreviewHtml(reader, "", page, true);
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "RainControl\nLevel: 21 l 52%\nTrue\nReason: Hold\n\n&lt;End&gt;\n"));
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "name='f5'"));
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "name='s5_3'"));
    TEST_ASSERT_NULL(strstr(html.c_str(), "name='f6'"));
    page.sources[1][1] = "sensor/99/value";
    html = buildPropertyPagePreviewHtml(reader, "", page, true);
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "[Line 2: Source is unknown"));
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "\nTrue\nReason: Hold\n"));
    TEST_ASSERT_NULL(strstr(html.c_str(), "Level: 21 l"));
    page.sources[1][0] = "";
    html = buildPropertyPagePreviewHtml(reader, "", page, true);
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "Fill sources in order without gaps"));
    const unsigned reads = reader.reads;
    html = buildPropertyPagePreviewHtml(reader, "", page, false);
    TEST_ASSERT_EQUAL_UINT32(reads, reader.reads);
    TEST_ASSERT_NULL(strstr(html.c_str(), "<pre"));
}

void test_load_saved_submits_an_independent_form() {
    OrderedReader reader;
    PropertyPreviewPage page;
    page.formats[0] = "Unsaved draft";
    const String html = buildPropertyPagePreviewHtml(reader, "", page, false);
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "<div class='actions'><button"));
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "<button type='submit' form='display-load'>Load saved settings</button>"));
    // Load must submit a separate form, without any draft fields or save action.
    const char* load = strstr(html.c_str(), "</form><form id='display-load' method='get' action='/measurements#text-preview'>");
    TEST_ASSERT_NOT_NULL(load);
    TEST_ASSERT_NOT_NULL(strstr(load, "<input type='hidden' name='loadDisplay' value='1'></form>"));
    TEST_ASSERT_NULL(strstr(load, "name='f0'"));
    TEST_ASSERT_NULL(strstr(load, "name='s0_0'"));
    TEST_ASSERT_NULL(strstr(html.c_str(), "<a href='/measurements#text-preview'>Load saved settings</a>"));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_load_saved_submits_an_independent_form);
    RUN_TEST(test_float_width_precision_alignment_and_literal_percent);
    RUN_TEST(test_integer_and_boolean_text);
    RUN_TEST(test_enum_uses_display_text_and_never_interprets_it_as_format);
    RUN_TEST(test_unsupported_formats_are_rejected_before_reading);
    RUN_TEST(test_type_mismatch_and_invalid_values);
    RUN_TEST(test_unknown_and_missing_sources_are_distinct);
    RUN_TEST(test_bounded_formats_and_output_never_return_partial_text);
    RUN_TEST(test_source_parser_requires_complete_bounded_reference);
    RUN_TEST(test_web_preview_keeps_selection_and_escapes_all_user_text);
    RUN_TEST(test_web_does_not_read_before_submission_and_handles_empty_inventory);
    RUN_TEST(test_multiple_ordered_sources_and_mixed_types);
    RUN_TEST(test_literal_and_empty_lines_require_no_sources);
    RUN_TEST(test_multi_source_errors_clear_whole_line_and_bound_total_output);
    RUN_TEST(test_six_line_page_preserves_layout_and_isolates_row_errors);
    return UNITY_END();
}
