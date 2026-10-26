// EnvNode native partition-table double: see repository LICENSE.
#pragma once
#include <string>
#include <set>
enum esp_partition_type_t { ESP_PARTITION_TYPE_DATA = 1 };
enum esp_partition_subtype_t { ESP_PARTITION_SUBTYPE_DATA_NVS = 2 };
struct esp_partition_t {};
inline std::set<std::string>& missingTestPartitions() { static std::set<std::string> labels; return labels; }
inline const esp_partition_t* esp_partition_find_first(esp_partition_type_t, esp_partition_subtype_t, const char* label) {
    static esp_partition_t partition;
    return label && missingTestPartitions().count(label)==0 ? &partition : nullptr;
}
