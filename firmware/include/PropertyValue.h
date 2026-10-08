#pragma once

#include "MeasurementValue.h"
#include <Arduino.h>
#include <memory>

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

// Static enums borrow metadata; configurable enums own their code and label.
struct OwnedPropertyEnum {
    String code;
    String label;
    PropertyEnumOption option;
    OwnedPropertyEnum(const String& c, const String& l)
        : code(c), label(l), option{code.c_str(), label.c_str()} {}
    OwnedPropertyEnum(const OwnedPropertyEnum&) = delete;
    OwnedPropertyEnum& operator=(const OwnedPropertyEnum&) = delete;
};
class PropertyValue {
public:
    PropertyValue() = default;
    PropertyValue(const MeasurementValue& value) : scalar_(value) {}
    static PropertyValue enumeration(const PropertyEnumOption& option) {
        PropertyValue result;
        result.enum_ = &option;
        return result;
    }
    static PropertyValue enumeration(const String& code, const String& label) {
        PropertyValue result;
        result.ownedEnum_ = std::make_shared<OwnedPropertyEnum>(code, label);
        result.enum_ = &result.ownedEnum_->option;
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
    std::shared_ptr<const OwnedPropertyEnum> ownedEnum_;
};

} // namespace EnvNode
