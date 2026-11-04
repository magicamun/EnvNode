// EnvNode-owned: see ../../../../LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
#pragma once
#include <math.h>
#include <stdint.h>

namespace EnvNode {

enum class RainEstimate : uint8_t { Unknown, Active, EndEstimated };

struct RainEvaluationConfig {
    float fastTimeSeconds = 0.25f;
    float slowTimeSeconds = 10.0f;
    float trendTimeSeconds = 5.0f;
    float minimumDropHz = 150.0f;
    float relativeDrop = 0.005f;
    float recoveryHzPerSecond = 3.0f;
    float recoveryFallToleranceHzPerSecond = 1.5f;
    uint32_t endConfirmationMs = 120000;
    uint32_t maximumGapMs = 1500;
    uint32_t heaterGuardMs = 5000;
    uint32_t eventCooldownMs = 2000;
};

struct RainEvaluation {
    bool valid = false;
    bool newWetting = false;
    float fastHz = 0;
    float slowHz = 0;
    float trendHzPerSecond = 0;
    RainEstimate estimate = RainEstimate::Unknown;
};

// No allocation, hardware access, logging, clock reads, or heater commands.
// The caller supplies monotonic milliseconds and the heater level for the window.
class RainDetectorEvaluation {
public:
    explicit RainDetectorEvaluation(const RainEvaluationConfig& config = RainEvaluationConfig())
        : config_(config) {}

    void reset() {
        initialized_ = false;
        result_ = RainEvaluation{};
        hasEvent_ = false;
        recovering_ = false;
    }

    RainEvaluation update(uint64_t timeMs, float frequencyHz, unsigned heaterPercent) {
        result_.newWetting = false;
        if (!validConfig() || !isfinite(frequencyHz) || frequencyHz <= 0
            || (heaterPercent != 0 && heaterPercent != 100)) {
            reset();
            return result_;
        }
        if (!initialized_ || timeMs <= lastTimeMs_
            || timeMs - lastTimeMs_ > config_.maximumGapMs) {
            reset();
            seed(timeMs, frequencyHz, heaterPercent);
            return result_;
        }
        const float dt = static_cast<float>(timeMs - lastTimeMs_) / 1000.0f;
        if (heaterPercent != lastHeater_) {
            // Retain a known rain estimate but rebuild filter baselines.
            seed(timeMs, frequencyHz, heaterPercent);
            return result_;
        }
        const float previousFast = result_.fastHz;
        result_.fastHz += dt / (config_.fastTimeSeconds + dt) * (frequencyHz - result_.fastHz);
        result_.slowHz += dt / (config_.slowTimeSeconds + dt) * (frequencyHz - result_.slowHz);
        const float velocity = (result_.fastHz - previousFast) / dt;
        result_.trendHzPerSecond += dt / (config_.trendTimeSeconds + dt)
            * (velocity - result_.trendHzPerSecond);
        lastTimeMs_ = timeMs;
        if (timeMs - seededAtMs_ < config_.heaterGuardMs) {
            result_.valid = false;
            recovering_ = false;
            return result_;
        }
        result_.valid = true;
        const float relativeThreshold = previousFast * config_.relativeDrop;
        const float threshold = relativeThreshold > config_.minimumDropHz
            ? relativeThreshold : config_.minimumDropHz;
        const bool drop = previousFast - result_.fastHz >= threshold;
        if (drop && (!hasEvent_ || timeMs - lastEventMs_ >= config_.eventCooldownMs)) {
            result_.newWetting = true;
            result_.estimate = RainEstimate::Active;
            lastEventMs_ = timeMs;
            hasEvent_ = true;
            recovering_ = false;
        }
        if (drop || result_.trendHzPerSecond < -config_.recoveryFallToleranceHzPerSecond) {
            recovering_ = false;
        } else if (result_.estimate == RainEstimate::Active) {
            if (!recovering_ && result_.trendHzPerSecond >= config_.recoveryHzPerSecond) {
                recoveryStartHz_ = result_.fastHz;
                recoverySinceMs_ = timeMs;
                recovering_ = true;
            }
            if (recovering_ && result_.fastHz - recoveryStartHz_ >= config_.minimumDropHz
                && hasEvent_ && timeMs - lastEventMs_ >= config_.endConfirmationMs
                && timeMs - recoverySinceMs_ >= config_.endConfirmationMs) {
                result_.estimate = RainEstimate::EndEstimated;
                recovering_ = false;
            }
        }
        return result_;
    }

private:
    bool validConfig() const {
        return isfinite(config_.fastTimeSeconds) && config_.fastTimeSeconds > 0
            && isfinite(config_.slowTimeSeconds) && config_.slowTimeSeconds > 0
            && isfinite(config_.trendTimeSeconds) && config_.trendTimeSeconds > 0
            && isfinite(config_.minimumDropHz) && config_.minimumDropHz > 0
            && isfinite(config_.relativeDrop) && config_.relativeDrop > 0
            && config_.relativeDrop < 1 && isfinite(config_.recoveryHzPerSecond)
            && config_.recoveryHzPerSecond > 0
            && isfinite(config_.recoveryFallToleranceHzPerSecond)
            && config_.recoveryFallToleranceHzPerSecond >= 0 && config_.endConfirmationMs > 0
            && config_.maximumGapMs > 0;
    }
    void seed(uint64_t timeMs, float frequencyHz, unsigned heaterPercent) {
        initialized_ = true;
        lastTimeMs_ = seededAtMs_ = timeMs;
        lastHeater_ = heaterPercent;
        result_.fastHz = result_.slowHz = frequencyHz;
        result_.trendHzPerSecond = 0;
        result_.valid = result_.newWetting = false;
        recovering_ = false;
    }
    RainEvaluationConfig config_;
    RainEvaluation result_;
    uint64_t lastTimeMs_ = 0;
    uint64_t seededAtMs_ = 0;
    uint64_t lastEventMs_ = 0;
    uint64_t recoverySinceMs_ = 0;
    float recoveryStartHz_ = 0;
    unsigned lastHeater_ = 0;
    bool initialized_ = false;
    bool hasEvent_ = false;
    bool recovering_ = false;
};

inline const char* rainEstimateName(RainEstimate estimate) {
    switch (estimate) {
        case RainEstimate::Active: return "active";
        case RainEstimate::EndEstimated: return "end_estimated";
        default: return "unknown";
    }
}
} // namespace EnvNode
