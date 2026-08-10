#pragma once

#include <cstdint>

namespace EnvNode {

enum class ValueKind {
    None,
    FloatingPoint,
    Boolean,
    UnsignedInteger,
};

class MeasurementValue {
public:
    MeasurementValue() : kind_(ValueKind::None), storage_() {}

    static MeasurementValue none() {
        return MeasurementValue();
    }

    static MeasurementValue floatingPoint(float value) {
        MeasurementValue result;
        result.kind_ = ValueKind::FloatingPoint;
        result.storage_.floatingPoint = value;
        return result;
    }

    static MeasurementValue boolean(bool value) {
        MeasurementValue result;
        result.kind_ = ValueKind::Boolean;
        result.storage_.boolean = value;
        return result;
    }

    static MeasurementValue unsignedInteger(uint32_t value) {
        MeasurementValue result;
        result.kind_ = ValueKind::UnsignedInteger;
        result.storage_.unsignedInteger = value;
        return result;
    }

    ValueKind kind() const {
        return kind_;
    }

    bool tryGetFloatingPoint(float& value) const {
        if (kind_ != ValueKind::FloatingPoint) {
            return false;
        }
        value = storage_.floatingPoint;
        return true;
    }

    bool tryGetBoolean(bool& value) const {
        if (kind_ != ValueKind::Boolean) {
            return false;
        }
        value = storage_.boolean;
        return true;
    }

    bool tryGetUnsignedInteger(uint32_t& value) const {
        if (kind_ != ValueKind::UnsignedInteger) {
            return false;
        }
        value = storage_.unsignedInteger;
        return true;
    }

private:
    union Storage {
        float floatingPoint;
        bool boolean;
        uint32_t unsignedInteger;

        Storage() : unsignedInteger(0) {}
    };

    ValueKind kind_;
    Storage storage_;
};

} // namespace EnvNode
