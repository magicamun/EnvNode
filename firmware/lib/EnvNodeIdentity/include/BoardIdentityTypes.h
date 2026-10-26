// EnvNode: see repository LICENSE.
#pragma once
#include <cstdint>
namespace EnvNode {
enum class BoardProfileId : uint8_t {
    EnvNodeMainboard = 0,
    EnvNodeWeather = 1,
    EnvNodeMini = 2,
};

struct BoardRevision {
    uint8_t major;
    uint8_t minor;
};

} // namespace EnvNode
