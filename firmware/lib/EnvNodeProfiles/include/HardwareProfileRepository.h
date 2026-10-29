// EnvNode: see repository LICENSE.
#pragma once
#include <memory>
#include "BoardPortMap.h"
namespace EnvNode {
constexpr size_t MaximumHardwareProfiles = 16;
constexpr size_t MaximumHardwareCatalogBytes = 16384;
constexpr size_t MaximumHardwareProfileBytes = 2048;
struct HardwareProfileEntry {
    char id[96] = {}, name[96] = {}, image[224] = {}, filename[176] = {}, sha256[65] = {};
    uint8_t kind = 0;
    BoardRevision revision = {};
    DescriptorSemanticVersion minimumFirmware;
    size_t bytes = 0;
};
struct HardwareProfileCatalog {
    size_t count = 0;
    HardwareProfileEntry entries[MaximumHardwareProfiles];
};
bool safeHardwareProfileFilename(const char*);
bool decodeHardwareProfileCatalog(const uint8_t*, size_t, HardwareProfileCatalog&);
class IHardwareProfileTransport {
public:
    virtual ~IHardwareProfileTransport() = default;
    // Implementations must verify HTTPS origin and SHA-256 when a digest is supplied.
    virtual bool fetch(const char* filename, size_t maximum, const char* digest,
        std::unique_ptr<uint8_t[]>& bytes, size_t& size) = 0;
};
struct LoadedHardwareProfile {
    HardwareProfileEntry entry;
    std::unique_ptr<uint8_t[]> bytes;
    size_t size = 0;
};
class HardwareProfileRepository {
public:
    explicit HardwareProfileRepository(IHardwareProfileTransport& transport) : transport_(transport) {}
    bool refresh();
    void refreshOnce() { if (!attempted_) refresh(); }
    size_t count() const { return catalog_ ? catalog_->count : 0; }
    const HardwareProfileEntry* at(size_t i) const { return i < count() ? &catalog_->entries[i] : nullptr; }
    const LoadedHardwareProfile* load(const char* id, const char* expectedDigest);
    const char* status() const { return status_; }
private:
    IHardwareProfileTransport& transport_;
    std::unique_ptr<HardwareProfileCatalog> catalog_;
    std::unique_ptr<LoadedHardwareProfile> loaded_;
    const char* status_ = "waiting";
    bool attempted_ = false;
};
}
