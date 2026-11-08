// EnvNode: see repository LICENSE.
#pragma once
#include "TestModel.h"
#include "MeasurementPolicy.h"
namespace EnvNode { namespace Testing {
enum class Phase : uint8_t { Empty, AwaitingConnection, Running, Review, Complete, Aborted };
enum class Result : uint8_t { Pending, Isolated, Connected, Unexpected, Open, Error, DirectedPending, DirectedConnected, DirectedBlocked };
enum class Verdict : uint8_t { Untested, Passed, Failed, Incomplete };
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
    Verdict verdict(size_t step) const;
    Verdict overall() const;
    Verdict contactVerdict(size_t step, size_t contact) const;
    const MeasurementPolicy* policy() const { return policy_; }
    uint8_t reverseEvidence(size_t rule) const { return rule<2?reverseEvidence_[rule]:0; }
    uint32_t generation() const { return generation_; }
private:
    bool select();
    void refreshVerdicts();
    bool positivePath(size_t connector,size_t contact) const;
    Result evaluated(size_t connector,size_t cell) const;
    const MeasurementPolicy* policy_ = nullptr;
    uint8_t reverseEvidence_[2] = {}; // 0 missing/invalid, 1 blocked with verified positive path, 2 conducting

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
