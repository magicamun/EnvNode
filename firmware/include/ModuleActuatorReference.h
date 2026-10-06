#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace EnvNode {

struct ModuleActuatorReference {
    uint8_t moduleInstanceFingerprint[8] = {};
    uint8_t deviceIdHash[4] = {};
};

inline void setModuleActuatorInstanceId(
    ModuleActuatorReference& reference, const uint8_t* instanceId) {
    memcpy(reference.moduleInstanceFingerprint, instanceId,
        sizeof(reference.moduleInstanceFingerprint));
}

inline void setModuleActuatorDeviceId(
    ModuleActuatorReference& reference, const char* deviceId) {
    uint32_t hash = 2166136261UL;
    if (deviceId != nullptr) {
        while (*deviceId != '\0') {
            hash ^= static_cast<uint8_t>(*deviceId++);
            hash *= 16777619UL;
        }
    }
    for (size_t index = 0; index < sizeof(reference.deviceIdHash); ++index) {
        reference.deviceIdHash[index] = static_cast<uint8_t>(hash >> (index * 8));
    }
}

inline bool validModuleActuatorReference(const ModuleActuatorReference& reference) {
    bool nonzero = false;
    for (size_t index = 0; index < sizeof(reference.moduleInstanceFingerprint); ++index) {
        nonzero = nonzero || reference.moduleInstanceFingerprint[index] != 0;
    }
    bool nonzeroDevice = false;
    for (size_t index = 0; index < sizeof(reference.deviceIdHash); ++index) {
        nonzeroDevice = nonzeroDevice || reference.deviceIdHash[index] != 0;
    }
    return nonzero && nonzeroDevice;
}

inline bool sameModuleActuatorReference(
    const ModuleActuatorReference& left,
    const ModuleActuatorReference& right) {
    if (!validModuleActuatorReference(left) || !validModuleActuatorReference(right)) {
        return false;
    }
    for (size_t index = 0; index < sizeof(left.moduleInstanceFingerprint); ++index) {
        if (left.moduleInstanceFingerprint[index]
            != right.moduleInstanceFingerprint[index]) return false;
    }
    return memcmp(left.deviceIdHash, right.deviceIdHash,
        sizeof(left.deviceIdHash)) == 0;
}

} // namespace EnvNode
