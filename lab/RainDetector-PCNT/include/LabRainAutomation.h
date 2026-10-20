// EnvNode-owned: see ../../../LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
#pragma once
#include <math.h>
#include <stdint.h>
#include "HeaterControl.h"

namespace RainLab {
struct AutoConfig {
    unsigned edgeMs = 1000;
    unsigned dropHz = 150;
    unsigned dropPermille = 5;
    unsigned endSeconds = 60;
    unsigned dryHz = 32000;
    unsigned hysteresisHz = 500;
    unsigned drySeconds = 10;
    bool valid() const {
        return edgeMs >= 250 && edgeMs <= 2000 && dropHz >= 20 && dropHz <= 10000
            && dropPermille >= 1 && dropPermille <= 200
            && endSeconds >= 30 && endSeconds <= 300
            && dryHz >= 1000 && dryHz <= 100000
            && hysteresisHz > 0 && hysteresisHz < dryHz
            && drySeconds >= 1 && drySeconds <= 120;
    }
};
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
enum class AutoRain { Unknown, Active, EndEstimated };
inline const char* autoRainName(AutoRain rain) {
    return rain == AutoRain::Active ? "active" : rain == AutoRain::EndEstimated
        ? "end_estimated" : "unknown";
}
struct AutoResult {
    bool valid = false;
    bool wetting = false;
    bool dry = false; // Latched completion of this cycle; no ambient dry inference.
    bool heating = false;
    AutoRain rain = AutoRain::Unknown;
    uint64_t quietMs = 0;
    uint64_t dryMs = 0;
};
// Hardware-free, bounded state; caller owns GPIO and time. Partial PWM is
// experimentally allowed for edge detection only; dry confirmation needs 100%.
class LabRainAutomation {
public:
    const AutoConfig& config() const { return config_; }
    const AutoResult& result() const { return result_; }
    bool enabled() const { return enabled_; }
    bool configure(const AutoConfig& config) {
        if (enabled_ || !config.valid()) return false;
        config_ = config;
        return true;
    }
    void enable() { disable(); enabled_ = true; }
    void disable() {
        enabled_ = false;
        initialized_ = false;
        count_ = 0;
        hasEvent_ = false;
        drying_ = false;
        result_ = AutoResult{};
    }
    AutoResult update(uint64_t timeMs, float hz, unsigned heater) {
        result_.wetting = false;
        result_.valid = false;
        if (!enabled_) return result_;
        if (!isfinite(hz) || hz <= 0 || heater > 100) {
            invalidate();
            return result_;
        }
        if (!initialized_ || timeMs <= lastMs_ || timeMs - lastMs_ > 1500) {
            if (initialized_) invalidate();
            initialized_ = true;
            lastMs_ = guardSinceMs_ = timeMs;
            lastHeater_ = heater;
            count_ = 0;
            return result_;
        }
        const uint64_t dt = timeMs - lastMs_;
        lastMs_ = timeMs;
        if (heater != lastHeater_) {
            lastHeater_ = heater;
            guardSinceMs_ = timeMs;
            count_ = 0;
            drying_ = false;
            result_.dryMs = 0;
            return result_;
        }
        if (timeMs - guardSinceMs_ < 5000) return result_;
        // Ignore tiny windows (e.g. interval switching) and unsupported rates.
        // The caller must submit normal windows only; 100-ms minimum bounds RAM.
        if (dt < 100) { invalidate(); return result_; }
        unsigned kept = 0;
        float peak = hz;
        for (unsigned i = 0; i < count_; ++i) {
            if (timeMs - history_[i].timeMs <= config_.edgeMs) {
                history_[kept++] = history_[i];
                if (history_[i].hz > peak) peak = history_[i].hz;
            }
        }
        count_ = kept;
        result_.valid = true;
        const float drop = peak - hz;
        const bool edge = count_ && drop >= config_.dropHz
            && drop >= peak * config_.dropPermille / 1000.0f;
        if (edge && (!hasEvent_ || timeMs - lastEventMs_ >= 2000)) {
            result_.wetting = true;
            result_.rain = AutoRain::Active;
            result_.heating = true;
            result_.dry = false;
            result_.quietMs = result_.dryMs = 0;
            drying_ = false;
            lastEventMs_ = timeMs;
            hasEvent_ = true;
        } else if (result_.rain == AutoRain::Active) {
            result_.quietMs += dt;
            if (result_.quietMs >= config_.endSeconds * 1000ULL)
                result_.rain = AutoRain::EndEstimated;
        }
        // A fresh wetting always restarts confirmation, even above dryHz.
        if (result_.heating && heater == 100 && !result_.wetting) {
            if (hz < config_.dryHz - config_.hysteresisHz) {
                drying_ = false;
                result_.dryMs = 0;
            } else if (!drying_ && hz >= config_.dryHz) {
                drying_ = true;
                result_.dryMs = 0;
            } else if (drying_) {
                result_.dryMs += dt;
                if (result_.dryMs >= config_.drySeconds * 1000ULL) {
                    result_.dry = true;
                    result_.heating = false;
                    drying_ = false;
                }
            }
        } else {
            drying_ = false;
            result_.dryMs = 0;
        }
        if (count_ == Capacity) { invalidate(); return result_; }
        history_[count_++] = Point{timeMs, hz};
        return result_;
    }
private:
    void invalidate() {
        initialized_ = false;
        count_ = 0;
        hasEvent_ = false;
        drying_ = false;
        result_.valid = false;
        result_.rain = AutoRain::Unknown;
        result_.quietMs = result_.dryMs = 0;
        result_.dry = false;
        // Abort heating on bad data. Remain armed; only a fresh edge may restart it.
        result_.heating = false;
    }
    struct Point { uint64_t timeMs; float hz; };
    static constexpr unsigned Capacity = 24;
    Point history_[Capacity]{};
    unsigned count_ = 0;
    AutoConfig config_;
    AutoResult result_;
    bool enabled_ = false, initialized_ = false, hasEvent_ = false, drying_ = false;
    unsigned lastHeater_ = 0;
    uint64_t lastMs_ = 0, guardSinceMs_ = 0, lastEventMs_ = 0;
};
} // namespace RainLab
