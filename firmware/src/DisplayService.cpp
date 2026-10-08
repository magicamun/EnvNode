#include "DisplayService.h"
#include "DisplayPageFormatter.h"
#include <cstdio>
#include <cstring>
namespace EnvNode {
DisplayService::DisplayService(const DisplayConfiguration& configuration, const IPropertyReader& properties,
    ITextDisplay& display, const IMonotonicClock& clock, ILogger& logger, ITextDisplay* secondDisplay)
    : configuration_(configuration), properties_(properties), displays_{&display, secondDisplay}, clock_(clock), logger_(logger) {}
void DisplayService::setStatus(size_t index, DisplayStatus status) {
    auto& status_ = states_[index].status_;
    if (status_ == status) return;
    status_ = status;
    if (status == DisplayStatus::Ready) logger_.infof("Display %u ready", static_cast<unsigned>(index + 1));
    else if (status == DisplayStatus::Unavailable) logger_.warnf("Display %u unavailable; retry in 10 seconds", static_cast<unsigned>(index + 1));
    else logger_.infof("Display %u disabled", static_cast<unsigned>(index + 1));
}
void DisplayService::loop() {
    // Release all changed targets before acquiring either, including a bus swap.
    for (size_t index = 0; index < 2; ++index) if (displays_[index]) prepare(index);
    for (size_t index = 0; index < 2; ++index) if (displays_[index]) render(index);
}
void DisplayService::prepare(size_t index) {
    auto& display_ = *displays_[index];
    auto& active_ = states_[index].active_;
    auto& haveConfiguration_ = states_[index].haveConfiguration_;
    auto& attempted_ = states_[index].attempted_;
    auto& haveFrame_ = states_[index].haveFrame_;
    auto& errorLines_ = states_[index].errorLines_;

    if (!haveConfiguration_ || !(active_ == configuration_.output(index))) {
        if (haveConfiguration_) {
            logger_.infof("OLED release: %s, %s, address=0x%02X",
                textDisplayTypeName(active_.type), i2cBusName(active_.bus), static_cast<unsigned>(active_.address));
            display_.end();
        }
        active_ = configuration_.output(index);
        haveConfiguration_ = true;
        attempted_ = false;
        haveFrame_ = false;
        errorLines_ = 0;
        setStatus(index, DisplayStatus::Disabled);
    }
}
void DisplayService::render(size_t index) {
    auto& display_ = *displays_[index];
    auto& active_ = states_[index].active_;
    auto& status_ = states_[index].status_;
    auto& attempted_ = states_[index].attempted_;
    auto& haveFrame_ = states_[index].haveFrame_;
    auto& lastAttempt_ = states_[index].lastAttempt_;
    auto& lastFrame_ = states_[index].lastFrame_;
    auto& errorLines_ = states_[index].errorLines_;
    const uint32_t now = clock_.nowMs();
    if (!active_.enabled) return;
    if (status_ != DisplayStatus::Ready) {
        if (attempted_ && static_cast<uint32_t>(now - lastAttempt_) < 10000) return;
        attempted_ = true;
        lastAttempt_ = now;
        logger_.infof("OLED initialize: %s, %s, address=0x%02X",
            textDisplayTypeName(active_.type), i2cBusName(active_.bus), static_cast<unsigned>(active_.address));
        if (!display_.begin(active_)) { setStatus(index, DisplayStatus::Unavailable); return; }
        setStatus(index, DisplayStatus::Ready);
        haveFrame_ = false;
    }
    if (haveFrame_ && static_cast<uint32_t>(now - lastFrame_) < 1000) return;
    lastFrame_ = now;
    haveFrame_ = true;
    TextDisplayFrame frame;
    uint8_t errors = 0;
    for (size_t line = 0; line < DisplayLineCount; ++line) {
        const auto result = formatDisplayLine(properties_, configuration_.page(configuration_.pageAssignment[index]), line);
        if (result.error != nullptr) {
            errors |= (1U << line);
            snprintf(frame.lines[line], sizeof(frame.lines[line]), "[Line %u error]", static_cast<unsigned>(line + 1));
            if (!(errorLines_ & (1U << line))) logger_.warnf("OLED line %u: %s", static_cast<unsigned>(line + 1), result.error);
        } else {
            strcpy(frame.lines[line], result.value.text);
        }
    }
    if (errorLines_ && !errors) logger_.info("OLED line errors cleared");
    errorLines_ = errors;
    if (!display_.show(frame)) {
        lastAttempt_ = now;
        setStatus(index, DisplayStatus::Unavailable);
    }
}
}
