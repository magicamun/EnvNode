// EnvNode: see repository LICENSE.
#pragma once
namespace EnvNode {
// Nonblocking ownership across web downloads, formatting, tests and recording.
void setSdRuntimeStable(bool stable);
bool sdRuntimeStable();
class SdStorageLease {
public:
    SdStorageLease();
    ~SdStorageLease();
    void release();
    explicit operator bool() const { return held_; }
    SdStorageLease(const SdStorageLease&)=delete;
    SdStorageLease& operator=(const SdStorageLease&)=delete;
private:
    bool held_;
};
}
