// EnvNode: see repository LICENSE.
#include "ConfigurationStorage.h"
#include <memory>
#include <new>
#include "Logger.h"
#include <cstring>
#include <esp_partition.h>
namespace EnvNode {
ConfigurationStorage::ConfigurationStorage(const ConfigurationStorageLayout& layout)
    : layout_(layout), activeNamespace_(layout.legacy) {}
bool ConfigurationStorage::begin() {
    if (!initialized_) {
        // A newly installed partition starts with legacy data still authoritative.
        // Switch only after a verified snapshot and its metadata have committed.
        if (layout_.snapshotPartition && esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                ESP_PARTITION_SUBTYPE_DATA_NVS, layout_.snapshotPartition)) {
            Preferences dedicatedMetadata;
            if (!dedicatedMetadata.begin(layout_.metadata, false, layout_.snapshotPartition)) return false;
            const uint32_t bank = dedicatedMetadata.getUInt("active", 2);
            dedicatedMetadata.end();
            if (bank <= 1) {
                activePartition_ = layout_.snapshotPartition;
                activeNamespace_ = bank == 0 ? layout_.bank0 : layout_.bank1;
                initialized_ = preferences_.begin(activeNamespace_.c_str(), false, activePartition_);
                return initialized_;
            }
        }
        Preferences metadata;
        if (!metadata.begin(layout_.metadata, false)) return false;
        const uint32_t bank = metadata.getUInt("active", 2);
        metadata.end();
        activePartition_ = nullptr;
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
bool ConfigurationStorage::replaceSnapshot(const uint8_t* data, size_t size, ILogger* logger) {
    if (logger) logger->info("Configuration storage selecting target bank");
    if (data == nullptr || size == 0) return false;
    begin();
    if (!initialized_) {
        if (logger) logger->error("Configuration storage is unavailable");
        return false;
    }
    const char* targetPartition = layout_.snapshotPartition;
    if (targetPartition && !esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
            ESP_PARTITION_SUBTYPE_DATA_NVS, targetPartition)) {
        if (logger) logger->error("Configuration storage partition missing; USB partition table update required");
        return false;
    }
    const uint32_t bank = activePartition_ == targetPartition && activeNamespace_ == layout_.bank0 ? 1 : 0;
    const char* nextNamespace = bank == 0 ? layout_.bank0 : layout_.bank1;
    if (logger) logger->infof("Configuration storage opening partition=%s bank=%s bytes=%u",
        targetPartition ? targetPartition : "nvs", nextNamespace, static_cast<unsigned>(size));
    Preferences staging;
    if (!staging.begin(nextNamespace, false, targetPartition)) return false;
    if (logger) logger->info("Configuration storage clearing inactive bank");
    if (!staging.clear()) return false;
    if (logger) logger->info("Configuration storage writing snapshot");
    bool written = staging.putBytes("cfg_snapshot", data, size) == size;
    if (logger) logger->infof("Configuration storage snapshot write success=%u", written ? 1u : 0u);
    if (written) {
        if (logger) logger->info("Configuration storage verifying snapshot");
        std::unique_ptr<uint8_t[]> bytes(new (std::nothrow) uint8_t[size]);
        written = bytes && staging.getBytesLength("cfg_snapshot") == size
            && staging.getBytes("cfg_snapshot", bytes.get(), size) == size
            && memcmp(bytes.get(), data, size) == 0;
    }
    staging.end();
    if (!written) return false;
    if (logger) logger->info("Configuration storage committing active bank");
    Preferences metadata;
    if (!metadata.begin(layout_.metadata, false, targetPartition)) return false;
    const bool committed = metadata.putUInt("active", bank) == sizeof(uint32_t);
    metadata.end();
    if (!committed) return false;
    if (logger) logger->info("Configuration storage reopening committed bank");
    preferences_.end();
    activePartition_ = targetPartition;
    activeNamespace_ = nextNamespace;
    initialized_ = preferences_.begin(nextNamespace, false, activePartition_);
    if (logger) logger->infof("Configuration storage committed bank reopened=%u", initialized_ ? 1u : 0u);
    return true;
}
bool ConfigurationStorage::clearPartition(const char* partition) {
    for (const char* name : {layout_.legacy, layout_.bank0, layout_.bank1}) {
        Preferences old;
        if (!old.begin(name, false, partition)) return false;
        const bool cleared = old.clear();
        old.end();
        if (!cleared) return false;
    }
    return true;
}
bool ConfigurationStorage::clearAll() {
    begin();
    if (!initialized_) return false;
    // Clear the other partition too, so legacy credentials cannot reappear.
    if (layout_.snapshotPartition) {
        if (activePartition_) {
            if (!clearPartition(nullptr)) return false;
        } else if (esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                ESP_PARTITION_SUBTYPE_DATA_NVS, layout_.snapshotPartition)
            && !clearPartition(layout_.snapshotPartition)) return false;
    }
    for (const char* name : {layout_.legacy, layout_.bank0, layout_.bank1}) {
        if (activeNamespace_ == name) continue;
        Preferences old;
        if (!old.begin(name, false, activePartition_)) return false;
        const bool cleared = old.clear(); old.end();
        if (!cleared) return false;
    }
    return preferences_.clear();
}
}
