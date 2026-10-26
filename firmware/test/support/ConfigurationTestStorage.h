// EnvNode: see repository LICENSE.
#pragma once
#include "Preferences.h"
#include "esp_partition.h"
inline void clearConfigurationTestStorage() {
    missingTestPartitions().clear();
    Preferences::failBeginNamespace() = String(); Preferences::failClearNamespace() = String();
    Preferences::failStringWrites() = false; Preferences::failByteReads() = false; Preferences::failUIntKey() = String();
    for (const char* partition : {"nvs", "cfg_nvs"}) {
        for (const char* name : {"weather", "weather0", "weather1", "weather_meta"}) {
            Preferences p; p.begin(name, false, partition); p.clear(); p.end();
        }
    }
}
