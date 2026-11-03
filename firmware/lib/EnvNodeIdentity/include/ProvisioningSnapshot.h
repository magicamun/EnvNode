// EnvNode: see repository LICENSE.
#pragma once
#include "BoardIdentityTypes.h"
namespace EnvNode {
enum class ProvisioningSnapshotStatus { Unavailable, Empty, Valid, Corrupt };
struct ProvisioningSnapshot {
    ProvisioningSnapshotStatus status = ProvisioningSnapshotStatus::Unavailable;
    char token[20] = {};
    char name[96] = {}, serial[65] = {}, batch[65] = {}, date[11] = {};
    uint8_t profileId = 255;
    BoardRevision revision = {};
};
const char* provisioningSnapshotStatusName(ProvisioningSnapshotStatus);
}
