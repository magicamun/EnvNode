#pragma once
#include "ITextDisplay.h"
#include "IMonotonicClock.h"
#include "Logger.h"
namespace EnvNode {
enum class DisplayStatus { Disabled, Unavailable, Ready };
class DisplayService {
public:
    DisplayService(const DisplayConfiguration& configuration, const IPropertyReader& properties,
        ITextDisplay& display, const IMonotonicClock& clock, ILogger& logger);
    void loop();
    DisplayStatus status() const { return status_; }
private:
    void setStatus(DisplayStatus status);
    const DisplayConfiguration& configuration_;
    const IPropertyReader& properties_;
    ITextDisplay& display_;
    const IMonotonicClock& clock_;
    ILogger& logger_;
    TextDisplayConfiguration active_;
    DisplayStatus status_ = DisplayStatus::Disabled;
    bool haveConfiguration_ = false;
    bool attempted_ = false;
    bool haveFrame_ = false;
    uint32_t lastAttempt_ = 0;
    uint32_t lastFrame_ = 0;
    uint8_t errorLines_ = 0;
};
}
