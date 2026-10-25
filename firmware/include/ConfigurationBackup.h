// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
// See the repository root LICENSE.
#pragma once
#include "Configuration.h"
#include <memory>
namespace EnvNode {
constexpr size_t MaxConfigurationBackupLength = 96 * 1024;
bool encodeConfigurationBackup(const Configuration& configuration, bool includePasswords,
    const char* firmwareVersion, String& output);
bool decodeConfigurationBackup(const String& input, const Configuration& current,
    Configuration& candidate, bool& includesPasswords);
// Web preview takes ownership of the decoded object, avoiding a second full model.
// On failure, candidate and includesPasswords remain unchanged.
bool decodeConfigurationBackupCandidate(const String& input, const Configuration& current,
    std::unique_ptr<Configuration>& candidate, bool& includesPasswords);
// Compact, versioned NVS snapshot. Field names stay in the JSON download only.
bool encodeConfigurationRecord(const Configuration& configuration, std::vector<uint8_t>& output);
bool decodeConfigurationRecord(const std::vector<uint8_t>& input, const Configuration& current,
    Configuration& candidate);
}
