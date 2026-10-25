// EnvNode: see repository LICENSE.
#pragma once
#include "BaseConfiguration.h"
namespace EnvNode {
class ITimeConfiguration {
public:
    virtual ~ITimeConfiguration() = default;
    virtual const TimeConfiguration& getTimeConfiguration() const = 0;
};
}
