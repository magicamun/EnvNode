// EnvNode: see ../../../LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
#pragma once

#include <stdint.h>
#include <string.h>

#include "LabConfig.h"

namespace RainLab {
enum class CommandKind { Invalid, Heat, HeatContinuous, Resume, Off, Status, Help };
struct Command {
    CommandKind kind = CommandKind::Invalid;
    unsigned percent = 0;
    unsigned seconds = 0;
};

inline void skipSpaces(const char*& text) {
    while (*text == ' ' || *text == '\t') ++text;
}

inline bool readNumber(const char*& text, unsigned limit, unsigned& value) {
    skipSpaces(text);
    if (*text < '0' || *text > '9') return false;
    value = 0;
    while (*text >= '0' && *text <= '9') {
        const unsigned digit = static_cast<unsigned>(*text++ - '0');
        if (value > limit / 10 || (value == limit / 10 && digit > limit % 10))
            return false;
        value = value * 10 + digit;
    }
    return true;
}

inline bool readWord(const char*& text, const char* word) {
    skipSpaces(text);
    const size_t length = strlen(word);
    if (strncmp(text, word, length) != 0) return false;
    const char next = text[length];
    if (next && next != ' ' && next != '\t') return false;
    text += length;
    return true;
}

inline Command parseCommand(const char* line) {
    Command command;
    if (readWord(line, "HEAT")) {
        if (readWord(line, "OFF")) command.kind = CommandKind::Off;
        else if (readWord(line, "ON")) command.kind = CommandKind::Resume;
        else {
            if (!readNumber(line, 100, command.percent) || command.percent == 0
                || (*line != ' ' && *line != '\t')) return Command{};
            if (readWord(line, "ON")) command.kind = CommandKind::HeatContinuous;
            else {
                if (!readNumber(line, LabConfig::HeaterMaxDurationSeconds, command.seconds)
                    || command.seconds == 0) return Command{};
                command.kind = CommandKind::Heat;
            }
        }
    } else if (readWord(line, "OFF")) command.kind = CommandKind::Off;
    else if (readWord(line, "STATUS")) command.kind = CommandKind::Status;
    else if (readWord(line, "HELP")) command.kind = CommandKind::Help;
    else return command;
    skipSpaces(line);
    return *line == '\0' ? command : Command{};
}

class HeaterControl {
public:
    void start(unsigned percent, unsigned seconds, int64_t nowUs) {
        percent_ = lastPercent_ = percent;
        timed_ = true;
        deadlineUs_ = nowUs + static_cast<int64_t>(seconds) * 1000000;
    }
    void startContinuous(unsigned percent) {
        percent_ = lastPercent_ = percent;
        timed_ = false;
        deadlineUs_ = 0;
    }
    bool resume() {
        if (lastPercent_ == 0) return false;
        startContinuous(lastPercent_);
        return true;
    }
    void off() { percent_ = 0; deadlineUs_ = 0; timed_ = false; }
    bool continuous() const { return percent_ != 0 && !timed_; }
    bool expire(int64_t nowUs) {
        if (percent_ == 0 || !timed_ || nowUs < deadlineUs_) return false;
        off();
        return true;
    }
    unsigned percent() const { return percent_; }
    int64_t remainingUs(int64_t nowUs) const {
        return percent_ != 0 && nowUs < deadlineUs_ ? deadlineUs_ - nowUs : 0;
    }
private:
    unsigned percent_ = 0;
    unsigned lastPercent_ = 0;
    bool timed_ = false;
    int64_t deadlineUs_ = 0;
};
}  // namespace RainLab
