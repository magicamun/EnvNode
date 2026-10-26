// EnvNode: see repository LICENSE.
#pragma once
#include <Preferences.h>
class ILogger;
namespace EnvNode {
struct ConfigurationStorageLayout {
    const char* legacy;
    const char* bank0;
    const char* bank1;
    const char* metadata;
    const char* snapshotPartition;
    constexpr ConfigurationStorageLayout(const char* legacyName, const char* firstBank,
        const char* secondBank, const char* metadataName, const char* partition = nullptr)
        : legacy(legacyName), bank0(firstBank), bank1(secondBank), metadata(metadataName),
          snapshotPartition(partition) {}
};
// The layout and its strings must outlive this store.
class ConfigurationStorage {
public:
    explicit ConfigurationStorage(const ConfigurationStorageLayout& layout);
    bool begin();
    bool ready() const { return initialized_; }
    Preferences& preferences() { return preferences_; }
    const String& activeNamespace() const { return activeNamespace_; }
    const char* activePartition() const { return activePartition_; }
    bool persistString(const char* key, const String& value);
    bool persistUInt(const char* key, uint32_t value);
    bool persistFloat(const char* key, float value);
    bool replaceSnapshot(const uint8_t* data, size_t size, ILogger* logger = nullptr);
    bool clearAll();
private:
    const ConfigurationStorageLayout& layout_;
    bool clearPartition(const char* partition);
    Preferences preferences_;
    const char* activePartition_ = nullptr;
    bool initialized_ = false;
    String activeNamespace_;
};
}
