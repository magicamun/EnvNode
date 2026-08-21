#pragma once

namespace EnvNode {

struct LinearTwoPointCalibration {
    float inputAtMin;
    float valueAtMin;
    float inputAtMax;
    float valueAtMax;
};

struct LinearCalibrationResult {
    LinearCalibrationResult() = default;
    LinearCalibrationResult(float value, bool valid)
        : value(value), valid(valid) {}

    float value = 0.0f;
    bool valid = false;
};

bool isValid(const LinearTwoPointCalibration& calibration);
LinearCalibrationResult applyLinearTwoPointCalibration(
    const LinearTwoPointCalibration& calibration,
    float input);

} // namespace EnvNode
