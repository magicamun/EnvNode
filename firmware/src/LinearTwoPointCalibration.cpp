#include "LinearTwoPointCalibration.h"

#include <cmath>

namespace EnvNode {

bool isValid(const LinearTwoPointCalibration& calibration) {
    return std::isfinite(calibration.inputAtMin)
        && std::isfinite(calibration.valueAtMin)
        && std::isfinite(calibration.inputAtMax)
        && std::isfinite(calibration.valueAtMax)
        && calibration.inputAtMin != calibration.inputAtMax;
}

LinearCalibrationResult applyLinearTwoPointCalibration(
    const LinearTwoPointCalibration& calibration,
    float input) {
    if (!isValid(calibration) || !std::isfinite(input)) return {};

    const float position = (input - calibration.inputAtMin)
        / (calibration.inputAtMax - calibration.inputAtMin);
    const float value = calibration.valueAtMin
        + position * (calibration.valueAtMax - calibration.valueAtMin);
    if (!std::isfinite(value)) return {};
    return {value, true};
}

} // namespace EnvNode
