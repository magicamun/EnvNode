// EnvNode: see repository LICENSE.
#pragma once
#include "INetworkConfiguration.h"
namespace EnvNode {
class INetworkSettings : public INetworkConfiguration {
public:
    virtual bool setNetworkConfiguration(const NetworkConfiguration& network, bool updatePassword) = 0;
};
}
