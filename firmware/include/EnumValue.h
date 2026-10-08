#pragma once

#include <Arduino.h>
#include <cstdint>
#include <vector>

namespace EnvNode {

using ValueId = uint16_t;
enum class ValueRestartPolicy : uint8_t { DefaultOnRestart, RestoreLastValue };
struct EnumValueOption {
    String code;
    String label;
};
struct EnumValueConfiguration {
    ValueId id = 0;
    String name;
    std::vector<EnumValueOption> options;
    String defaultCode;
    ValueRestartPolicy restartPolicy = ValueRestartPolicy::DefaultOnRestart;
};

bool validEnumValueConfiguration(const EnumValueConfiguration& configuration);
enum class EnumValueStartResult { DefaultSelected, Restored, SavedValueRejected, InvalidConfiguration };
enum class EnumValueSetResult { Changed, Unchanged, UnknownOption, NotStarted };

// Owns its definition and state. Transport and persistence belong to services.
class EnumValue {
public:
    explicit EnumValue(const EnumValueConfiguration& configuration);
    EnumValueStartResult begin(const String& savedCode = String());
    EnumValueSetResult set(const String& code);
    const EnumValueConfiguration& configuration() const { return configuration_; }
    // Borrowed until this instance is destroyed; null before a successful begin.
    const EnumValueOption* current() const;
    uint32_t revision() const { return revision_; }
private:
    int find(const String& code) const;
    EnumValueConfiguration configuration_;
    int selected_ = -1;
    uint32_t revision_ = 0;
};

} // namespace EnvNode
