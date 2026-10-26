// EnvNode: see repository LICENSE.
#include "TestRunner.h"
#include <algorithm>
namespace EnvNode { namespace Testing {
const Connector* TestRunner::connector() const {
    return board_ && step_ < board_->count ? &board_->connectors[step_] : nullptr;
}
Result TestRunner::result(size_t i) const { return i < MaxContacts * SocketPins ? results_[i] : Result::Error; }
Verdict TestRunner::verdict(size_t i) const { return i < MaxConnectors ? verdicts_[i] : Verdict::Failed; }
bool TestRunner::load(const BoardDefinition& board) {
    if (phase_ == Phase::Running || !board.socket || !board.connectors || !board.count || board.count > MaxConnectors) return false;
    for (size_t i = 0; i < board.count; ++i) {
        const auto& c = board.connectors[i];
        if (!c.contacts || c.count != (c.pigtail == Pigtail::Module ? MaxContacts : 4)) return false;
        for (size_t p = 0; p < c.count; ++p)
            if (c.contacts[p].number != p + 1 || (c.contacts[p].expected >> SocketPins)) return false;
    }
    for (size_t p = 0; p < SocketPins; ++p) if (board.socket[p].number != p + 1) return false;
    for (size_t i = 0; i < MaxConnectors; ++i) std::fill_n(contactVerdicts_[i], MaxContacts, Verdict::Untested);
    fixture_.release(); board_ = &board; step_ = completed_ = 0;
    std::fill_n(results_, MaxContacts * SocketPins, Result::Pending);
    std::fill_n(verdicts_, MaxConnectors, Verdict::Untested);
    phase_ = Phase::AwaitingConnection; ++generation_; return true;
}
bool TestRunner::start(uint32_t now) {
    if (phase_ != Phase::AwaitingConnection || !connector()) return false;
    std::fill_n(results_, MaxContacts * SocketPins, Result::Pending);
    completed_ = 0; lastSample_ = now; phase_ = Phase::Running; ++generation_;
    pathReady_ = select(); return true;
}
bool TestRunner::select() {
    const bool selected = fixture_.select(connector()->pigtail, static_cast<uint8_t>(completed_ / SocketPins + 1),
        static_cast<uint8_t>(completed_ % SocketPins + 1));
    if (!selected) fixture_.release();
    return selected;
}
void TestRunner::tick(uint32_t now) {
    if (phase_ != Phase::Running || uint32_t(now - lastSample_) < 20) return;
    bool connected = false;
    auto sample = pathReady_ ? fixture_.read(connected) : ITestFixture::Sample::Error;
    if (sample == ITestFixture::Sample::Busy) {
        if (uint32_t(now - lastSample_) < 2000) return;
        sample = ITestFixture::Sample::Error;
    }
    if (sample == ITestFixture::Sample::Error) fixture_.release();
    const bool expected = (connector()->contacts[completed_ / SocketPins].expected >> (completed_ % SocketPins)) & 1;
    results_[completed_] = sample == ITestFixture::Sample::Error ? Result::Error
        : connected ? (expected ? Result::Connected : Result::Unexpected)
                    : (expected ? Result::Open : Result::Isolated);
    ++completed_; lastSample_ = now;
    if (completed_ == connector()->count * SocketPins) finish();
    else pathReady_ = select();
}
void TestRunner::finish() {
    fixture_.release(); bool failed = false, incomplete = false;
    for (size_t i = 0; i < completed_; ++i)
        failed |= results_[i] == Result::Unexpected || results_[i] == Result::Open || results_[i] == Result::Error;
    for (size_t i = 0; i < connector()->count; ++i) incomplete |= connector()->contacts[i].expected == 0;
    for (size_t p = 0; p < connector()->count; ++p) {
        bool rowFailed = false;
        for (size_t s = 0; s < SocketPins; ++s) rowFailed |= results_[p * SocketPins + s] >= Result::Unexpected;
        contactVerdicts_[step_][p] = rowFailed ? Verdict::Failed
            : connector()->contacts[p].expected ? Verdict::Passed : Verdict::Incomplete;
    }
    verdicts_[step_] = failed ? Verdict::Failed : incomplete ? Verdict::Incomplete : Verdict::Passed;
    phase_ = Phase::Review; ++generation_;
}
bool TestRunner::next() {
    if (phase_ != Phase::Review) return false;
    ++step_; completed_ = 0;
    std::fill_n(results_, MaxContacts * SocketPins, Result::Pending);
    phase_ = step_ < board_->count ? Phase::AwaitingConnection : Phase::Complete;
    ++generation_; return true;
}
void TestRunner::abort() { fixture_.release(); phase_ = Phase::Aborted; ++generation_; }
Verdict TestRunner::contactVerdict(size_t step, size_t contact) const {
    return step < MaxConnectors && contact < MaxContacts ? contactVerdicts_[step][contact] : Verdict::Failed;
}
Verdict TestRunner::overall() const {
    if (!board_) return Verdict::Untested;
    bool incomplete = phase_ != Phase::Complete;
    for (size_t i = 0; i < board_->count; ++i) {
        if (verdicts_[i] == Verdict::Failed) return Verdict::Failed;
        incomplete |= verdicts_[i] != Verdict::Passed;
    }
    return incomplete ? Verdict::Incomplete : Verdict::Passed;
}
} }
