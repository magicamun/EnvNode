// EnvNode: see repository LICENSE.
#pragma once
namespace EnvNode {
// ESP-IDF 4.4 FAT VFS keeps only ESP_VFS_PATH_MAX-2 path characters.
// A longer path can register successfully but cannot be found for unregister.
constexpr char SdCardMountPath[] = "/sd";
}
