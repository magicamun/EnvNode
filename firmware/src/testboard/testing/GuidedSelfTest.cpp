// EnvNode: see repository LICENSE.
#include "GuidedSelfTest.h"
namespace EnvNode { namespace Testing {
MuxAddress GuidedSelfTest::output(size_t index) {
    const auto& r = fixtureRevision01().routing;
    return index < 4 ? r.gpio[index] : index < 8 ? r.i2c[index-4] : r.module[index-8];
}
bool GuidedSelfTest::start(size_t index, uint32_t now, uint16_t settleMs) {
    if (running_ || index >= Count || (settleMs!=15 && settleMs!=50 && settleMs!=100)) return false;
    selected_ = index; results_[index] = SelfTestResult();
    results_[index].settleMs = settleMs;
    results_[index].verdict = SelfTestVerdict::Running;
    running_ = true; failed_ = false; step_ = 0; startedAt_ = now;
    switchStep(now); return true;
}
void GuidedSelfTest::cancel() {
    if (!running_) return;
    running_ = false;
    results_[selected_].verdict = SelfTestVerdict::Aborted;
    io_.releaseDiagnostic();
}
void GuidedSelfTest::switchStep(uint32_t now) {
    const bool inputTest = selected_ >= OutputCount;
    const auto& r = fixtureRevision01().routing;
    DiagnosticPath p{inputTest ? r.module[1] : output(selected_), -1, 0, step_ != 0};
    if (inputTest) {
        auto input = r.socket[selected_-OutputCount];
        if (step_ >= 3) input = r.socket[step_-3];
        if (step_ != 2) { p.inputDevice = input.device; p.inputChannel = input.channel; }
    } else if (step_ >= 3) {
        p.output = output(step_-3);
    }
    // Step 2 explicitly verifies the disabled selected multiplexer path.
    if (step_ == 2 && !inputTest) p.output.device = 255;
    io_.selectDiagnostic(p);
    switchedAt_ = sampledAt_ = now; samples_ = above_ = 0; sum_ = 0; minimum_ = 4095; maximum_ = 0;
}
void GuidedSelfTest::tick(uint32_t now) {
    if (!running_) return;
    if (uint32_t(now-startedAt_) > 10000) { cancel(); return; }
    // Let RC and mux paths settle; sample one value per application turn.
    if (uint32_t(now-switchedAt_) < results_[selected_].settleMs || (samples_ && uint32_t(now-sampledAt_) < 2)) return;
    const uint16_t value = io_.sampleDiagnostic(); sampledAt_ = now;
    windowSamples_[samples_] = value;
    if(value > LowMaximum) ++above_;
    sum_ += value; if (value < minimum_) minimum_ = value; if (value > maximum_) maximum_ = value;
    if (++samples_ < 8) return;
    auto& result = results_[selected_];
    const uint16_t mean = uint16_t(sum_/samples_);
    if (step_ == 0) { result.low = mean; failed_ |= maximum_ > LowMaximum; }
    else if (step_ == 1) {
        result.high = mean; failed_ |= minimum_ < HighMinimum || maximum_ > HighMaximum;
    } else {
        result.isolationTotalSamples += samples_;
        result.isolationTotalAbove += above_;
        if(above_) ++result.isolationBadWindows;
        if(result.isolationPath==255 || maximum_>result.isolation ||
            (maximum_==result.isolation && mean>result.isolationMean)) {
            result.isolation = maximum_;
            result.isolationPath = step_==2 ? 0 : uint8_t(step_-2);
            result.isolationMean = mean; result.isolationMinimum = minimum_;
            result.isolationAbove = above_;
            for(size_t i=0;i<8;++i)result.isolationSamples[i]=windowSamples_[i];
        }
        failed_ |= maximum_ > LowMaximum;
    }
    ++step_;
    const size_t channels = selected_ < OutputCount ? OutputCount : MaxInputs;
    // Skip the connected channel while testing all other mapped channels.
    if (step_ >= 3 && step_-3 == (selected_ < OutputCount ? selected_ : selected_-OutputCount)) ++step_;
    if (step_ >= 3+channels) {
        running_ = false; result.verdict = failed_ ? SelfTestVerdict::Failed : SelfTestVerdict::Passed;
        io_.releaseDiagnostic();
    } else switchStep(now);
}
} }
