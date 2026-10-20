// EnvNode-owned: see ../../LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
#pragma once
#include "IRainFrequencyInput.h"
#include "SensorId.h"
namespace EnvNode {
struct RainFrequencySnapshot {
    RainFrequencyWindow window;
    uint32_t acceptedMs = 0;
    uint32_t revision = 0;
};
class IRainFrequencyResolver {
public:
    virtual ~IRainFrequencyResolver() = default;
    virtual void rainFrequencyHeaterChanged(SensorId) {}
    virtual bool rainFrequencySnapshot(SensorId id, RainFrequencySnapshot& result) const = 0;
};
} // namespace EnvNode
