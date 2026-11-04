// EnvNode-owned: see ../../LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
#pragma once
#include "IRainFrequencyInput.h"

namespace EnvNode {
class IRainFrequencyObserver {
public:
    virtual ~IRainFrequencyObserver() = default;
    // Synchronous delivery of EVERY completed gate, including invalid data.
    // Observer must copy what it needs; references expire on return.
    virtual void onRainFrequency(const RainFrequencyWindow& window) = 0;
};

// Sensor-side acquisition lifecycle. No heater decisions or protocol publication.
// A controller may observe each gate locally; slower publication reads latest().
class RainFrequencyAcquisition {
public:
    RainFrequencyAcquisition(IRainFrequencyInput& input, IRainFrequencyObserver& observer)
        : input_(input), observer_(observer) {}
    ~RainFrequencyAcquisition() { stop(); }
    RainFrequencyAcquisition(const RainFrequencyAcquisition&) = delete;
    RainFrequencyAcquisition& operator=(const RainFrequencyAcquisition&) = delete;
    bool begin(uint8_t gpio) {
        stop();
        running_ = input_.begin(gpio);
        if (!running_) publish(RainFrequencyWindow{});
        return running_;
    }
    void service() {
        if (!running_) return;
        RainFrequencyWindow window;
        switch (input_.poll(window)) {
            case RainFrequencyPoll::Pending: return;
            case RainFrequencyPoll::Window: publish(window); return;
            case RainFrequencyPoll::Failed:
                input_.stop(); running_ = false;
                publish(RainFrequencyWindow{});
                return;
        }
    }
    void stop() {
        input_.stop();
        running_ = false;
        observed_ = false;
        last_ = RainFrequencyWindow{};
    }
    bool running() const { return running_; }
    bool latest(RainFrequencyWindow& window) const {
        if (!observed_) return false;
        window = last_;
        return true;
    }
private:
    void publish(const RainFrequencyWindow& window) {
        last_ = window;
        observed_ = true;
        observer_.onRainFrequency(last_);
    }
    IRainFrequencyInput& input_;
    IRainFrequencyObserver& observer_;
    RainFrequencyWindow last_;
    bool running_ = false;
    bool observed_ = false;
};
} // namespace EnvNode
