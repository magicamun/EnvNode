#pragma once

namespace EnvNode {

enum class AnalogInputError {
    None,
    NotInitialized,
    ReadFailed,
};

struct AnalogSample {
    AnalogSample() = default;
    AnalogSample(float voltage, bool valid, AnalogInputError error)
        : voltage(voltage), valid(valid), error(error) {}

    float voltage = 0.0f;
    bool valid = false;
    AnalogInputError error = AnalogInputError::None;
};

class IAnalogInput {
public:
    virtual ~IAnalogInput() = default;

    virtual bool begin() = 0;
    virtual AnalogSample read() = 0;
    virtual bool initialized() const = 0;
};

} // namespace EnvNode
