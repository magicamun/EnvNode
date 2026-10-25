// EnvNode: see repository LICENSE.
#include "ConfigurationStorage.h"
#include <vector>
#include <cstring>
namespace EnvNode {
ConfigurationStorage::ConfigurationStorage(const ConfigurationStorageLayout& layout)
    : layout_(layout), activeNamespace_(layout.legacy) {}
bool ConfigurationStorage::begin() {
    if (!initialized_) {
        Preferences metadata;
        if (!metadata.begin(layout_.metadata, false)) return false;
        const uint32_t bank = metadata.getUInt("active", 2);
        metadata.end();
        activeNamespace_ = bank == 0 ? layout_.bank0 : bank == 1 ? layout_.bank1 : layout_.legacy;
        initialized_ = preferences_.begin(activeNamespace_.c_str(), false);
    }
    return initialized_;
}
bool ConfigurationStorage::persistString(const char* key, const String& value) {
    begin();
    if (!initialized_) return false;
    // Empty values must override a restored snapshot, rather than expose its baseline again.
    if (value.isEmpty()) {
        preferences_.putString(key, value);
        return preferences_.isKey(key) && preferences_.getString(key).isEmpty();
    }
    return preferences_.putString(key, value) == value.length();
}
bool ConfigurationStorage::persistUInt(const char* key, uint32_t value) {
    begin();
    return preferences_.putUInt(key, value);
}
bool ConfigurationStorage::persistFloat(const char* key, float value) {
    begin();
    return preferences_.putFloat(key, value) > 0;
}
bool ConfigurationStorage::replaceSnapshot(const uint8_t* data, size_t size) {
    if (data == nullptr || size == 0) return false;
    begin();
    if (!initialized_) return false;
    const uint32_t bank = activeNamespace_ == layout_.bank0 ? 1 : 0;
    const char* nextNamespace = bank == 0 ? layout_.bank0 : layout_.bank1;
    Preferences staging;
    if (!staging.begin(nextNamespace, false)) return false;
    bool written = staging.clear()
        && staging.putBytes("cfg_snapshot", data, size) == size;
    if (written) {
        std::vector<uint8_t> bytes(size);
        written = staging.getBytesLength("cfg_snapshot") == size
            && staging.getBytes("cfg_snapshot", bytes.data(), size) == size
            && memcmp(bytes.data(), data, size) == 0;
    }
    staging.end();
    if (!written) return false;
    Preferences metadata;
    if (!metadata.begin(layout_.metadata, false)) return false;
    const bool committed = metadata.putUInt("active", bank) == sizeof(uint32_t);
    metadata.end();
    if (!committed) return false;
    preferences_.end();
    activeNamespace_ = nextNamespace;
    initialized_ = preferences_.begin(nextNamespace, false);
    return true;
}
bool ConfigurationStorage::clearAll() {
    begin();
    if (!initialized_) return false;
    // Erase inactive snapshots and legacy credentials too; keep the pointer to the cleared active bank.
    for (const char* name : {layout_.legacy, layout_.bank0, layout_.bank1}) {
        if (activeNamespace_ == name) continue;
        Preferences old;
        if (!old.begin(name, false)) return false;
        const bool cleared = old.clear(); old.end();
        if (!cleared) return false;
    }
    if (!preferences_.clear()) {
        return false;
    }
    return true;
}
}
