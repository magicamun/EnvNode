#pragma once

#include <cstdint>

namespace EnvNode {

class ActuatorLevel {
public:
    static constexpr uint8_t Minimum = 0;
    static constexpr uint8_t Maximum = 100;

    ActuatorLevel()
        : percentage_(Minimum) {
    }

    static bool tryCreate(uint8_t percentage, ActuatorLevel& level) {
        if (percentage > Maximum) return false;
        level = ActuatorLevel(percentage);
        return true;
    }

    static ActuatorLevel off() { return ActuatorLevel(Minimum); }
    static ActuatorLevel full() { return ActuatorLevel(Maximum); }

    uint8_t percent() const { return percentage_; }

    bool operator==(ActuatorLevel other) const {
        return percentage_ == other.percentage_;
    }

    bool operator!=(ActuatorLevel other) const { return !(*this == other); }

private:
    explicit ActuatorLevel(uint8_t percentage)
        : percentage_(percentage) {
    }

    uint8_t percentage_;
};

} // namespace EnvNode
