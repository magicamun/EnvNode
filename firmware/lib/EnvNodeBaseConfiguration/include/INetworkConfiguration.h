// EnvNode: see repository LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
#pragma once
#include "BaseConfiguration.h"
namespace EnvNode {
// Read-only view. The application owns validation, storage and lifetime.
class INetworkConfiguration {
public:
    virtual ~INetworkConfiguration() = default;
    virtual const NetworkConfiguration& getNetworkConfiguration() const = 0;
};
} // namespace EnvNode
