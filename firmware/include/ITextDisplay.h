#pragma once
#include "DisplayConfiguration.h"
namespace EnvNode {
struct TextDisplayFrame {
    char lines[DisplayLineCount][MaxPropertyTextLength + 1] = {};
};
class ITextDisplay {
public:
    virtual ~ITextDisplay() = default;
    virtual bool begin(const TextDisplayConfiguration& configuration) = 0;
    virtual bool show(const TextDisplayFrame& frame) = 0;
    virtual void end() = 0;
};
}
