// EnvNode: see repository LICENSE.
#pragma once
namespace EnvNode {
// The application decides how to report success/failure and activate changes.
class INetworkSaveObserver {
public:
    virtual ~INetworkSaveObserver() = default;
    virtual void networkSettingsSaved(bool success) = 0;
};
}
