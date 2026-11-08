// EnvNode: see repository LICENSE.
#pragma once
#include "TestModel.h"
#include "MeasurementPolicy.h"
namespace EnvNode { namespace Testing {
enum class Phase : uint8_t { Empty, AwaitingConnection, Running, Review, Complete, Aborted };
enum class Result : uint8_t { Pending, Isolated, Connected, Unexpected, Open, Error, DirectedPending, DirectedConnected, DirectedBlocked };
enum class Verdict : uint8_t { Untested, Passed, Failed, Incomplete };
struct MeasurementRecord {
    uint32_t attempt=0,atMs=0;
    uint16_t cell=0;
    uint8_t connector=0;
    Result result=Result::Error;
    MeasurementTrace trace;
};
class TestRunner {
public:
    explicit TestRunner(ITestFixture& fixture) : fixture_(fixture) {}
    bool load(const BoardDefinition& board);
    bool start(uint32_t now);
    bool resetForModeChange();
    bool next();
    bool selectConnector(size_t index);
    void abort();
    void tick(uint32_t now);
    Phase phase() const { return phase_; }
    const BoardDefinition* board() const { return board_; }
    const Connector* connector() const;
    size_t step() const { return step_; }
    size_t completed() const { return completed_; }
    Result result(size_t index) const;
    Result resultAt(size_t connector,size_t cell) const;
    Result rawResultAt(size_t connector,size_t cell) const;
    size_t completedAt(size_t connector) const {return connector<MaxConnectors?completedByConnector_[connector]:0;}
    static constexpr size_t DiagnosticCapacity=128;
    size_t diagnosticCount() const {return diagnosticCount_;}
    uint32_t diagnosticsDropped() const {return diagnosticsDropped_;}
    const MeasurementRecord& diagnosticAt(size_t index) const {return diagnostics_[(diagnosticNext_+DiagnosticCapacity-diagnosticCount_+index)%DiagnosticCapacity];}
    Verdict verdict(size_t step) const;
    Verdict overall() const;
    Verdict contactVerdict(size_t step, size_t contact) const;
    const MeasurementPolicy* policy() const { return policy_; }
    uint8_t reverseEvidence(size_t rule) const { return rule<MaxDirectedRules?reverseEvidence_[rule]:0; }
    uint32_t generation() const { return generation_; }
private:
    void recordDiagnostic(Result result,uint32_t now,MeasurementReason fallback);
    MeasurementRecord diagnostics_[DiagnosticCapacity];
    size_t diagnosticCount_=0,diagnosticNext_=0;
    uint32_t diagnosticsDropped_=0;
    bool select();
    void refreshVerdicts();
    bool positivePath(size_t connector,size_t contact) const;
    Result evaluated(size_t connector,size_t cell) const;
    const MeasurementPolicy* policy_ = nullptr;
    uint8_t reverseEvidence_[MaxDirectedRules] = {}; // 0 missing/invalid, 1 blocked with verified positive path, 2 conducting

    void finish();
    ITestFixture& fixture_;
    const BoardDefinition* board_ = nullptr;
    Phase phase_ = Phase::Empty;
    size_t step_ = 0, completed_ = 0;
    uint32_t lastSample_ = 0, generation_ = 0;
    bool pathReady_ = false;
    size_t inputCount() const { return board_ ? board_->inputCount : 0; }
    Result results_[MaxConnectors][MaxContacts * MaxInputs] = {};
    uint16_t completedByConnector_[MaxConnectors] = {};
    Verdict verdicts_[MaxConnectors] = {};
    Verdict contactVerdicts_[MaxConnectors][MaxContacts] = {};
};
} }
