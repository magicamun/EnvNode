#pragma once
#include "ITextDisplay.h"
#include "IMonotonicClock.h"
#include "Logger.h"
namespace EnvNode {
enum class DisplayStatus { Disabled, Unavailable, Ready };
class DisplayService {
public:
    DisplayService(const DisplayConfiguration& configuration, const IPropertyReader& properties,
        ITextDisplay& display, const IMonotonicClock& clock, ILogger& logger, ITextDisplay* secondDisplay = nullptr);
    void loop();
    DisplayStatus status(size_t index = 0) const { return states_[index < 2 ? index : 0].status_; }
private:
    void setStatus(size_t index, DisplayStatus status);
    void prepare(size_t index);
    void render(size_t index);
    const DisplayConfiguration& configuration_;
    const IPropertyReader& properties_;
    ITextDisplay* displays_[2];
    const IMonotonicClock& clock_;
    ILogger& logger_;
    struct State {
        TextDisplayConfiguration active_;
        DisplayStatus status_ = DisplayStatus::Disabled;
        bool haveConfiguration_ = false;
        bool attempted_ = false;
        bool haveFrame_ = false;
        uint32_t lastAttempt_ = 0;
        uint32_t lastFrame_ = 0;
        uint8_t errorLines_ = 0;
    };
    State states_[2];
};
}
