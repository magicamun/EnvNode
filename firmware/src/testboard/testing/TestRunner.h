// EnvNode: see repository LICENSE.
#pragma once
#include "TestModel.h"
namespace EnvNode { namespace Testing {
enum class Phase : uint8_t { Empty, AwaitingConnection, Running, Review, Complete, Aborted };
enum class Result : uint8_t { Pending, Isolated, Connected, Unexpected, Open, Error };
enum class Verdict : uint8_t { Untested, Passed, Failed, Incomplete };
class TestRunner {
public:
    explicit TestRunner(ITestFixture& fixture) : fixture_(fixture) {}
    bool load(const BoardDefinition& board);
    bool start(uint32_t now);
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
    uint32_t generation() const { return generation_; }
private:
    bool select();
    void finish();
    ITestFixture& fixture_;
    const BoardDefinition* board_ = nullptr;
    Phase phase_ = Phase::Empty;
    size_t step_ = 0, completed_ = 0;
    uint32_t lastSample_ = 0, generation_ = 0;
    bool pathReady_ = false;
    Result results_[MaxConnectors][MaxContacts * SocketPins] = {};
    uint16_t completedByConnector_[MaxConnectors] = {};
    Verdict verdicts_[MaxConnectors] = {};
    Verdict contactVerdicts_[MaxConnectors][MaxContacts] = {};
};
} }
