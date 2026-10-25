// EnvNode: see repository LICENSE.
#pragma once
#include <Preferences.h>
namespace EnvNode {
struct ConfigurationStorageLayout {
    const char* legacy;
    const char* bank0;
    const char* bank1;
    const char* metadata;
};
// The layout and its strings must outlive this store.
class ConfigurationStorage {
public:
    explicit ConfigurationStorage(const ConfigurationStorageLayout& layout);
    bool begin();
    bool ready() const { return initialized_; }
    Preferences& preferences() { return preferences_; }
    const String& activeNamespace() const { return activeNamespace_; }
    bool persistString(const char* key, const String& value);
    bool persistUInt(const char* key, uint32_t value);
    bool persistFloat(const char* key, float value);
    bool replaceSnapshot(const uint8_t* data, size_t size);
    bool clearAll();
private:
    const ConfigurationStorageLayout& layout_;
    Preferences preferences_;
    bool initialized_ = false;
    String activeNamespace_;
};
}
