// EnvNode: see repository LICENSE.
#pragma once
#include <cstdint>
namespace EnvNode {
// ESP-IDF 4.4 FAT VFS keeps only ESP_VFS_PATH_MAX-2 path characters.
// A longer path can register successfully but cannot be found for unregister.
constexpr char SdCardMountPath[] = "/sd";
// Every leased operation opens at most one regular file at a time.
constexpr uint8_t SdCardOpenFileLimit=1;
}
