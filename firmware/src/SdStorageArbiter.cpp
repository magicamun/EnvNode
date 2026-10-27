// EnvNode: see repository LICENSE.
#include "SdStorageArbiter.h"
#include <atomic>
namespace EnvNode {
namespace { std::atomic_flag owned=ATOMIC_FLAG_INIT; std::atomic<bool> runtimeStable{true}; }
void setSdRuntimeStable(bool stable) {runtimeStable.store(stable);}
bool sdRuntimeStable() {return runtimeStable.load();}
SdStorageLease::SdStorageLease() : held_(!owned.test_and_set(std::memory_order_acquire)) {}
SdStorageLease::~SdStorageLease() {release();}
void SdStorageLease::release() {if(held_) {held_=false;owned.clear(std::memory_order_release);}}
}
