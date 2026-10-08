#include "DisplayService.h"
#include "DisplayPageFormatter.h"
#include <cstdio>
#include <cstring>
namespace EnvNode {
DisplayService::DisplayService(const DisplayConfiguration& configuration, const IPropertyReader& properties,
    ITextDisplay& display, const IMonotonicClock& clock, ILogger& logger)
    : configuration_(configuration), properties_(properties), display_(display), clock_(clock), logger_(logger) {}
void DisplayService::setStatus(DisplayStatus status) {
    if (status_ == status) return;
    status_ = status;
    if (status == DisplayStatus::Ready) logger_.info("OLED display ready");
    else if (status == DisplayStatus::Unavailable) logger_.warn("OLED unavailable; retry in 10 seconds");
    else logger_.info("OLED display disabled");
}
void DisplayService::loop() {
    const uint32_t now = clock_.nowMs();
    if (!haveConfiguration_ || !(active_ == configuration_.hardware)) {
        if (haveConfiguration_) {
            logger_.infof("OLED release: %s, %s, address=0x%02X",
                textDisplayTypeName(active_.type), i2cBusName(active_.bus), static_cast<unsigned>(active_.address));
            display_.end();
        }
        active_ = configuration_.hardware;
        haveConfiguration_ = true;
        attempted_ = false;
        haveFrame_ = false;
        errorLines_ = 0;
        setStatus(DisplayStatus::Disabled);
    }
    if (!active_.enabled) return;
    if (status_ != DisplayStatus::Ready) {
        if (attempted_ && static_cast<uint32_t>(now - lastAttempt_) < 10000) return;
        attempted_ = true;
        lastAttempt_ = now;
        logger_.infof("OLED initialize: %s, %s, address=0x%02X",
            textDisplayTypeName(active_.type), i2cBusName(active_.bus), static_cast<unsigned>(active_.address));
        if (!display_.begin(active_)) { setStatus(DisplayStatus::Unavailable); return; }
        setStatus(DisplayStatus::Ready);
        haveFrame_ = false;
    }
    if (haveFrame_ && static_cast<uint32_t>(now - lastFrame_) < 1000) return;
    lastFrame_ = now;
    haveFrame_ = true;
    TextDisplayFrame frame;
    uint8_t errors = 0;
    for (size_t line = 0; line < DisplayLineCount; ++line) {
        const auto result = formatDisplayLine(properties_, configuration_, line);
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
        setStatus(DisplayStatus::Unavailable);
    }
}
}
