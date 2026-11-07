// EnvNode: see repository LICENSE.
#pragma once
#include "FixtureRouting.h"
namespace EnvNode { namespace Testing {
struct DiagnosticPath { MuxAddress output; // device 255: drive HIGH with both output muxes disabled.
    int inputDevice; uint8_t inputChannel; bool high; };
class IDiagnosticIo {
public:
    virtual ~IDiagnosticIo() = default;
    virtual void selectDiagnostic(const DiagnosticPath& path) = 0;
    virtual uint16_t sampleDiagnostic() = 0;
    virtual void releaseDiagnostic() = 0;
};
enum class SelfTestVerdict : uint8_t { Untested, Running, Passed, Failed, Aborted };
struct SelfTestResult {
    SelfTestVerdict verdict = SelfTestVerdict::Untested;
    uint16_t low = 0, high = 0, isolation = 0;
    uint16_t settleMs = 15;
    // 255: no completed isolation window, 0: disabled path, otherwise 1-based contact in its group.
    uint8_t isolationPath = 255, isolationAbove = 0, isolationBadWindows = 0;
    uint16_t isolationMean = 0, isolationMinimum = 0;
    uint16_t isolationTotalAbove = 0, isolationTotalSamples = 0;
    uint16_t isolationSamples[8] = {};

};
// Controller 0.1 + original produced Input-Adapter 0.1 only. Not a DUT profile.
class GuidedSelfTest {
public:
    static constexpr size_t OutputCount = 22, Count = OutputCount + MaxInputs;
    static constexpr uint16_t LowMaximum = 200, HighMinimum = 1000, HighMaximum = 3800;
    explicit GuidedSelfTest(IDiagnosticIo& io) : io_(io) {}
    bool start(size_t index, uint32_t now, uint16_t settleMs = 15);
    void tick(uint32_t now);
    void cancel();
    void clear() { if (!running_) for (auto& result : results_) result = SelfTestResult(); }
    bool running() const { return running_; }
    size_t selected() const { return selected_; }
    const SelfTestResult& result(size_t index) const { return results_[index]; }
    static MuxAddress output(size_t index);
private:
    void switchStep(uint32_t now);
    IDiagnosticIo& io_;
    SelfTestResult results_[Count];
    size_t selected_ = 0, step_ = 0;
    uint32_t switchedAt_ = 0, sampledAt_ = 0, startedAt_ = 0, sum_ = 0;
    uint16_t minimum_ = 4095, maximum_ = 0;
    uint8_t samples_ = 0, above_ = 0;
    uint16_t windowSamples_[8] = {};
    bool running_ = false, failed_ = false;
};
} }
