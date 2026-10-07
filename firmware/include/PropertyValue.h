#pragma once

#include "MeasurementValue.h"
#include <Arduino.h>

namespace EnvNode {

enum class PropertyValueKind { None, FloatingPoint, Boolean, UnsignedInteger, Enumeration, Text };

inline PropertyValueKind propertyValueKind(ValueKind kind) {
    switch (kind) {
        case ValueKind::FloatingPoint: return PropertyValueKind::FloatingPoint;
        case ValueKind::Boolean: return PropertyValueKind::Boolean;
        case ValueKind::UnsignedInteger: return PropertyValueKind::UnsignedInteger;
        default: return PropertyValueKind::None;
    }
}

struct PropertyEnumOption {
    const char* stableCode;
    const char* displayText;
};

// Reuse measurement scalar storage; enum metadata has static lifetime.
class PropertyValue {
public:
    PropertyValue() = default;
    PropertyValue(const MeasurementValue& value) : scalar_(value) {}
    static PropertyValue enumeration(const PropertyEnumOption& option) {
        PropertyValue result;
        result.enum_ = &option;
        return result;
    }
    static PropertyValue text(const String& text) {
        PropertyValue result;
        result.text_ = text;
        result.hasText_ = true;
        return result;
    }
    bool tryGetText(const char*& text) const {
        if (!hasText_) return false;
        text = text_.c_str();
        return true;
    }
    PropertyValueKind kind() const {
        if (hasText_) return PropertyValueKind::Text;
        return enum_ == nullptr ? propertyValueKind(scalar_.kind()) : PropertyValueKind::Enumeration;
    }
    bool tryGetFloatingPoint(float& value) const { return !hasText_ && enum_ == nullptr && scalar_.tryGetFloatingPoint(value); }
    bool tryGetBoolean(bool& value) const { return !hasText_ && enum_ == nullptr && scalar_.tryGetBoolean(value); }
    bool tryGetUnsignedInteger(uint32_t& value) const { return !hasText_ && enum_ == nullptr && scalar_.tryGetUnsignedInteger(value); }
    bool tryGetEnumeration(const PropertyEnumOption*& option) const {
        if (enum_ == nullptr) return false;
        option = enum_;
        return true;
    }
private:
    MeasurementValue scalar_;
    String text_;
    bool hasText_ = false;
    const PropertyEnumOption* enum_ = nullptr;
};

} // namespace EnvNode
