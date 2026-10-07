#include "PropertyTextFormatter.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace EnvNode {
namespace {

struct Conversion {
    size_t start = 0;
    size_t end = 0;
    char type = 0;
    int width = 0;
    int precision = 6;
    bool left = false;
    bool zero = false;
};

bool digit(char c) { return c >= '0' && c <= '9'; }

bool parse(const char* format, size_t length, Conversion* conversions, size_t& count) {
    count = 0;
    for (size_t i = 0; i < length; ++i) {
        if (static_cast<unsigned char>(format[i]) < 32 || format[i] == 127) return false;
        if (format[i] != '%') continue;
        if (format[i + 1] == '%') { ++i; continue; }
        if (count == MaxPropertySourcesPerLine) return false;
        Conversion& conversion = conversions[count++];
        conversion.start = i++;
        while (format[i] == '-' || format[i] == '0') {
            bool& flag = format[i] == '-' ? conversion.left : conversion.zero;
            if (flag) return false;
            flag = true;
            ++i;
        }
        while (digit(format[i])) {
            conversion.width = conversion.width * 10 + format[i++] - '0';
            if (conversion.width > 64) return false;
        }
        bool hasPrecision = false;
        if (format[i] == '.') {
            ++i;
            hasPrecision = true;
            conversion.precision = 0;
            if (!digit(format[i])) return false;
            while (digit(format[i])) {
                conversion.precision = conversion.precision * 10 + format[i++] - '0';
                if (conversion.precision > 6) return false;
            }
        }
        conversion.type = format[i];
        if (conversion.type != 'f' && conversion.type != 'u' && conversion.type != 's') return false;
        if (hasPrecision && conversion.type != 'f') return false;
        if (conversion.zero && conversion.type == 's') return false;
        conversion.end = i + 1;
    }
    return true;
}

PropertyTextResult failure(PropertyFormatStatus status) {
    PropertyTextResult result;
    result.status = status;
    return result;
}

bool accepts(char conversion, PropertyValueKind kind) {
    return (conversion == 'f' && kind == PropertyValueKind::FloatingPoint)
        || (conversion == 'u' && kind == PropertyValueKind::UnsignedInteger)
        || (conversion == 's' && (kind == PropertyValueKind::Boolean || kind == PropertyValueKind::Enumeration));
}

} // namespace

namespace {
PropertyTextResult formatValue(const IPropertyReader& reader, const PropertyReference& reference,
    const Conversion& conversion) {
    PropertyDescription description;
    if (!reader.describe(reference, description)) return failure(PropertyFormatStatus::UnknownReference);
    if (!accepts(conversion.type, description.valueKind)) return failure(PropertyFormatStatus::TypeMismatch);
    PropertySnapshot snapshot;
    const PropertyReadResult status = reader.read(reference, snapshot);
    if (status == PropertyReadResult::UnknownReference) return failure(PropertyFormatStatus::UnknownReference);
    if (status == PropertyReadResult::NoValue) return failure(PropertyFormatStatus::NoValue);
    if (!snapshot.valid || snapshot.value.kind() != description.valueKind) return failure(PropertyFormatStatus::InvalidValue);

    char formatted[MaxPropertyTextLength + 1] = {};
    int count = 0;
    if (conversion.type == 'f') {
        float number = 0;
        if (!snapshot.value.tryGetFloatingPoint(number) || !std::isfinite(number)) return failure(PropertyFormatStatus::InvalidValue);
        const char* numericFormat = conversion.left ? "%-*.*f" : conversion.zero ? "%0*.*f" : "%*.*f";
        count = snprintf(formatted, sizeof(formatted), numericFormat,
            conversion.width, conversion.precision, static_cast<double>(number));
    } else if (conversion.type == 'u') {
        uint32_t number = 0;
        if (!snapshot.value.tryGetUnsignedInteger(number)) return failure(PropertyFormatStatus::InvalidValue);
        const char* numericFormat = conversion.left ? "%-*lu" : conversion.zero ? "%0*lu" : "%*lu";
        count = snprintf(formatted, sizeof(formatted), numericFormat, conversion.width, static_cast<unsigned long>(number));
    } else {
        bool flag = false;
        const PropertyEnumOption* option = nullptr;
        const char* text = nullptr;
        if (snapshot.value.tryGetBoolean(flag)) text = flag ? description.trueText : description.falseText;
        else if (snapshot.value.tryGetEnumeration(option)) text = option->displayText;
        if (text == nullptr) return failure(PropertyFormatStatus::InvalidValue);
        size_t textLength = 0;
        while (textLength <= MaxPropertyTextLength && text[textLength] != '\0') {
            if (static_cast<unsigned char>(text[textLength]) < 32 || text[textLength] == 127) return failure(PropertyFormatStatus::InvalidValue);
            ++textLength;
        }
        if (textLength > MaxPropertyTextLength) return failure(PropertyFormatStatus::OutputTooLong);
        count = snprintf(formatted, sizeof(formatted), conversion.left ? "%-*s" : "%*s", conversion.width, text);
    }
    if (count < 0) return failure(PropertyFormatStatus::InvalidValue);
    if (static_cast<size_t>(count) > MaxPropertyTextLength) return failure(PropertyFormatStatus::OutputTooLong);

    PropertyTextResult result;
    memcpy(result.text, formatted, count + 1);
    result.status = PropertyFormatStatus::Formatted;
    return result;
}
} // namespace

bool validatePropertyFormat(const char* format, size_t sourceCount) {
    if (format == nullptr || sourceCount > MaxPropertySourcesPerLine) return false;
    const size_t length = strlen(format);
    if (length > MaxPropertyFormatLength) return false;
    Conversion conversions[MaxPropertySourcesPerLine];
    size_t count = 0;
    return parse(format, length, conversions, count) && count == sourceCount;
}

PropertyTextResult formatPropertyText(const IPropertyReader& reader,
    const PropertyReference* references, size_t referenceCount, const char* format) {
    if (format == nullptr) return failure(PropertyFormatStatus::InvalidFormat);
    size_t length = 0;
    while (length <= MaxPropertyFormatLength && format[length] != '\0') ++length;
    if (length > MaxPropertyFormatLength) return failure(PropertyFormatStatus::FormatTooLong);
    Conversion conversions[MaxPropertySourcesPerLine];
    size_t count = 0;
    if (!parse(format, length, conversions, count)) return failure(PropertyFormatStatus::InvalidFormat);
    if (referenceCount > MaxPropertySourcesPerLine || count != referenceCount
        || (referenceCount > 0 && references == nullptr)) return failure(PropertyFormatStatus::SourceCountMismatch);

    PropertyTextResult result;
    size_t used = 0;
    size_t next = 0;
    for (size_t i = 0; i < length;) {
        if (next < count && i == conversions[next].start) {
            const PropertyTextResult value = formatValue(reader, references[next], conversions[next]);
            if (value.status != PropertyFormatStatus::Formatted) return failure(value.status);
            const size_t valueLength = strlen(value.text);
            if (used + valueLength > MaxPropertyTextLength) return failure(PropertyFormatStatus::OutputTooLong);
            memcpy(result.text + used, value.text, valueLength);
            used += valueLength;
            i = conversions[next++].end;
        } else {
            if (used == MaxPropertyTextLength) return failure(PropertyFormatStatus::OutputTooLong);
            result.text[used++] = format[i++];
            if (result.text[used - 1] == '%') ++i; // parse() verified literal %%.
        }
    }
    result.text[used] = '\0';
    result.status = PropertyFormatStatus::Formatted;
    return result;
}

PropertyTextResult formatPropertyText(const IPropertyReader& reader,
    const PropertyReference& reference, const char* format) {
    PropertyTextResult result = formatPropertyText(reader, &reference, 1, format);
    if (result.status == PropertyFormatStatus::SourceCountMismatch) result.status = PropertyFormatStatus::InvalidFormat;
    return result;
}

const char* propertyFormatError(PropertyFormatStatus status) {
    switch (status) {
        case PropertyFormatStatus::Formatted: return "";
        case PropertyFormatStatus::InvalidFormat: return "Use up to four %f, %u or %s placeholders; %% prints a percent sign. Width: 0-64, decimal places for %f: 0-6. One line only.";
        case PropertyFormatStatus::FormatTooLong: return "Format is too long (maximum 128 bytes).";
        case PropertyFormatStatus::TypeMismatch: return "Format does not match the source type: %f for decimals, %u for unsigned integers, %s for Boolean or enum text.";
        case PropertyFormatStatus::UnknownReference: return "Source is unknown or no longer available.";
        case PropertyFormatStatus::NoValue: return "No value available yet.";
        case PropertyFormatStatus::InvalidValue: return "The current source value is invalid.";
        case PropertyFormatStatus::SourceCountMismatch: return "The number of sources must match the placeholders (maximum four). Literal text needs no sources.";
        case PropertyFormatStatus::OutputTooLong: return "Formatted line is too long (maximum 128 bytes).";
    }
    return "Unable to format this value.";
}

} // namespace EnvNode
