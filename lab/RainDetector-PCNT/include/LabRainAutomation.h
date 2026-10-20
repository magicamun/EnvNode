// EnvNode-owned: see ../../../LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
#pragma once
#include <RainDetectorPolicy.h>
#include "HeaterControl.h"

namespace RainLab {
// Console compatibility only. Lab and production use the same policy implementation.
using AutoConfig = EnvNode::RainDetectorPolicyConfig;
using AutoResult = EnvNode::RainDetectorPolicyResult;
using AutoRain = EnvNode::RainDetectorRainState;
using LabRainAutomation = EnvNode::RainDetectorPolicy;
inline const char* autoRainName(AutoRain rain) { return EnvNode::rainDetectorRainStateName(rain); }
// Integer-only, atomic configuration command, independently testable on host.
inline bool parseAutoConfig(const char* line, AutoConfig& result) {
    AutoConfig candidate;
    if (!readWord(line, "AUTO") || !readWord(line, "CONFIG")) return false;
    unsigned* fields[] = {&candidate.edgeMs, &candidate.dropHz, &candidate.dropPermille,
        &candidate.endSeconds, &candidate.dryHz, &candidate.hysteresisHz, &candidate.drySeconds};
    for (auto field : fields) {
        if (*line != ' ' && *line != '\t') return false;
        if (!readNumber(line, 100000, *field)) return false;
    }
    skipSpaces(line);
    if (*line || !candidate.valid()) return false;
    result = candidate;
    return true;
}
inline bool autoWord(const char* line, const char* word) {
    if (!readWord(line, "AUTO") || !readWord(line, word)) return false;
    skipSpaces(line);
    return *line == '\0';
}
} // namespace RainLab
