// EnvNode: see repository LICENSE.
#pragma once
namespace EnvNode {
// Configuration and board discovery must finish before the runtime starts.
// Hooks preserve the boundary between device work and network administration.
class IFirmwareApplication {
public:
    virtual ~IFirmwareApplication() = default;
    virtual void initializeDevices() = 0;
    virtual void beginIntegrations() = 0;
    virtual void serviceMeasurements() = 0;
    virtual void serviceIntegrations() = 0;
};
}
