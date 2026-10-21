// EnvNode-owned: see ../../../../LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
#pragma once
#include <math.h>
#include <stdint.h>

namespace EnvNode {
struct RainDetectorPolicyConfig {
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
enum class RainDetectorRainState { Unknown, Active, EndEstimated };
inline const char* rainDetectorRainStateName(RainDetectorRainState rain) {
    return rain == RainDetectorRainState::Active ? "active" : rain == RainDetectorRainState::EndEstimated
        ? "end_estimated" : "unknown";
}
struct RainDetectorPolicyResult {
    bool valid = false;
    bool wetting = false;
    bool dry = false; // Latched confirmation after startup check or a drying cycle.
    bool heating = false;
    RainDetectorRainState rain = RainDetectorRainState::Unknown;
    uint64_t quietMs = 0;
    uint64_t dryMs = 0;
};
// Hardware-free, bounded state; caller owns GPIO and time. Partial PWM is
// experimentally allowed for edge detection only; active drying needs 100%.
class RainDetectorPolicy {
public:
    const RainDetectorPolicyConfig& config() const { return config_; }
    const RainDetectorPolicyResult& result() const { return result_; }
    bool enabled() const { return enabled_; }
    bool configure(const RainDetectorPolicyConfig& config) {
        if (enabled_ || !config.valid()) return false;
        config_ = config;
        return true;
    }
    bool startupPending() const { return startupPending_; }
    void enable(bool initializeDry = true) { disable(); enabled_ = true; startupPending_ = initializeDry; }
    void disable() {
        enabled_ = false;
        initialized_ = false;
        count_ = 0;
        hasEvent_ = false;
        drying_ = false;
        startupPending_ = startupDryConfirmation_ = false;
        result_ = RainDetectorPolicyResult{};
    }
    RainDetectorPolicyResult update(uint64_t timeMs, float hz, unsigned heater) {
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
            startupPending_ = startupDryConfirmation_ = false;
            result_.wetting = true;
            result_.rain = RainDetectorRainState::Active;
            result_.heating = true;
            result_.dry = false;
            result_.quietMs = result_.dryMs = 0;
            drying_ = false;
            lastEventMs_ = timeMs;
            hasEvent_ = true;
        } else if (result_.rain == RainDetectorRainState::Active) {
            result_.quietMs += dt;
            if (result_.quietMs >= config_.endSeconds * 1000ULL)
                result_.rain = RainDetectorRainState::EndEstimated;
        }
        // One startup classification after the normal valid-signal guard. It
        // never fabricates rain. Bad input before this point simply keeps waiting.
        if (startupPending_ && !result_.wetting) {
            startupPending_ = false;
            startupDryConfirmation_ = hz >= config_.dryHz;
            result_.heating = !startupDryConfirmation_;
        }
        // A high startup value still needs confirmation. If it drops before that
        // confirmation, complete this initialization by drying at full power.
        if (startupDryConfirmation_ && hz < config_.dryHz) {
            startupDryConfirmation_ = false;
            drying_ = false;
            result_.dryMs = 0;
            result_.heating = true;
        }
        // A fresh wetting always restarts confirmation, even above dryHz.
        if (((result_.heating && heater == 100)
                || (startupDryConfirmation_ && heater == 0)) && !result_.wetting) {
            if (hz < config_.dryHz - config_.hysteresisHz) {
                drying_ = false;
                result_.dryMs = 0;
            } else if (!drying_ && hz >= config_.dryHz) {
                drying_ = true;
                result_.dryMs = 0;
            } else if (drying_) {
                result_.dryMs += dt;
                if (result_.dryMs >= config_.drySeconds * 1000ULL) {
                    startupDryConfirmation_ = false;
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
        result_.rain = RainDetectorRainState::Unknown;
        result_.quietMs = result_.dryMs = 0;
        result_.dry = false;
        startupDryConfirmation_ = false;
        // Abort heating on bad data. A consumed startup check is not rearmed;
        // only a fresh edge may restart heating after recovery.
        result_.heating = false;
    }
    struct Point { uint64_t timeMs; float hz; };
    static constexpr unsigned Capacity = 24;
    Point history_[Capacity]{};
    unsigned count_ = 0;
    RainDetectorPolicyConfig config_;
    RainDetectorPolicyResult result_;
    bool enabled_ = false, initialized_ = false, hasEvent_ = false, drying_ = false;
    bool startupPending_ = false, startupDryConfirmation_ = false;
    unsigned lastHeater_ = 0;
    uint64_t lastMs_ = 0, guardSinceMs_ = 0, lastEventMs_ = 0;
};
} // namespace EnvNode
