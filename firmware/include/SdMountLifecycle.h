// EnvNode: see repository LICENSE.
#pragma once
#include <cstdint>
namespace EnvNode {
enum class SdMountState { Disabled, NoCard, Waiting, Mounted, Ejecting, Ejected, MountFailed };
class ISdMountDevice {
public:
    virtual ~ISdMountDevice() = default;
    virtual bool mount() = 0;
    virtual void unmount() = 0;
};
// Single worker, called only while holding the exclusive storage lease.
class SdMountLifecycle {
public:
    void service(ISdMountDevice& device, bool enabled, bool present,
                 bool eject, bool drained, uint32_t now) {
        if (!enabled || !present) {
            detach(device);
            ejected_ = false; detecting_ = false; attempted_ = false;
            state_ = enabled ? SdMountState::NoCard : SdMountState::Disabled;
            return;
        }
        if (ejected_) { state_ = SdMountState::Ejected; return; }
        if (eject) {
            state_ = SdMountState::Ejecting;
            if (drained) { detach(device); ejected_ = true; state_ = SdMountState::Ejected; }
            return;
        }
        if (mounted_) { state_ = SdMountState::Mounted; return; }
        if (!detecting_) { detecting_ = true; detectedAt_ = now; }
        state_ = SdMountState::Waiting;
        if (now-detectedAt_ < 300) return; // insertion debounce
        if (attempted_ && now-attemptAt_ < 30000) { state_ = SdMountState::MountFailed; return; }
        attempted_ = true; attemptAt_ = now;
        mounted_ = device.mount();
        if (!mounted_) device.unmount(); // release partially initialized driver
        state_ = mounted_ ? SdMountState::Mounted : SdMountState::MountFailed;
    }
    void detach(ISdMountDevice& device) {
        if (mounted_) { device.unmount(); mounted_ = false; ++generation_; }
    }
    // Formatting is an explicitly exclusive operation; normal clients never detach.
    void invalidate(ISdMountDevice& device) { detach(device); detecting_=attempted_=false; state_=SdMountState::Waiting; }
    bool remountAfterMaintenance(ISdMountDevice& device,uint32_t now) {
        attempted_=true;attemptAt_=now;detecting_=true;detectedAt_=now;
        mounted_=device.mount();if(!mounted_)device.unmount();
        state_=mounted_?SdMountState::Mounted:SdMountState::MountFailed;
        return mounted_;
    }
    bool mounted() const { return mounted_; }
    uint32_t generation() const { return generation_; }
    SdMountState state() const { return state_; }
private:
    SdMountState state_ = SdMountState::Disabled;
    bool mounted_=false,ejected_=false,detecting_=false,attempted_=false;
    uint32_t detectedAt_=0,attemptAt_=0,generation_=0;
};
}
