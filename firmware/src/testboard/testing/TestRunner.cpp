// EnvNode: see repository LICENSE.
#include "TestRunner.h"
#include <algorithm>
#include <cstring>
namespace EnvNode { namespace Testing {
const Connector* TestRunner::connector() const {
    return board_ && step_ < board_->count ? &board_->connectors[step_] : nullptr;
}
Result TestRunner::result(size_t i) const { return step_ < MaxConnectors && i < MaxContacts * MaxInputs ? evaluated(step_,i) : Result::Error; }
Verdict TestRunner::verdict(size_t i) const { return i < MaxConnectors ? verdicts_[i] : Verdict::Failed; }
bool TestRunner::load(const BoardDefinition& board) {
    if (phase_ == Phase::Running || !board.socket || !board.connectors || !board.count || board.count > MaxConnectors
        || (board.inputCount != SocketPins && board.inputCount != MaxInputs)) return false;
    for (size_t i = 0; i < board.count; ++i) {
        const auto& c = board.connectors[i];
        if (!c.contacts || c.count != (c.pigtail == Pigtail::Module ? MaxContacts : 4)) return false;
        for (size_t p = 0; p < c.count; ++p)
            if (c.contacts[p].number != p + 1 || (c.contacts[p].expected >> board.inputCount)) return false;
    }
    for (size_t p = 0; p < board.inputCount; ++p) if (board.socket[p].number != p + 1) return false;
    for (size_t i = 0; i < MaxConnectors; ++i) std::fill_n(contactVerdicts_[i], MaxContacts, Verdict::Untested);
    fixture_.release(); board_ = &board; policy_=measurementPolicy(board);
    std::fill_n(reverseEvidence_,MaxDirectedRules,0); step_ = completed_ = 0;
    for (size_t i = 0; i < MaxConnectors; ++i) std::fill_n(results_[i], MaxContacts * MaxInputs, Result::Pending);
    std::fill_n(completedByConnector_, MaxConnectors, 0);
    std::fill_n(verdicts_, MaxConnectors, Verdict::Untested);
    phase_ = Phase::AwaitingConnection; ++generation_; return true;
}
bool TestRunner::resetForModeChange() {
    if(phase_==Phase::Running)return false;
    if(board_)return load(*board_);
    fixture_.release();phase_=Phase::Empty;++generation_;return true;
}
bool TestRunner::start(uint32_t now) {
    if ((phase_ != Phase::AwaitingConnection && phase_ != Phase::Review) || !connector()) return false;
    std::fill_n(results_[step_], MaxContacts * MaxInputs, Result::Pending);
    std::fill_n(contactVerdicts_[step_], MaxContacts, Verdict::Untested);
    verdicts_[step_] = Verdict::Untested;
    completedByConnector_[step_] = 0;
    completed_ = 0; lastSample_ = now; phase_ = Phase::Running; ++generation_;
    refreshVerdicts();pathReady_ = select(); return true;
}
bool TestRunner::select() {
    const bool selected = fixture_.select(connector()->pigtail, static_cast<uint8_t>(completed_ / inputCount() + 1),
        static_cast<uint8_t>(completed_ % inputCount() + 1));
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
    const auto& contact=connector()->contacts[completed_/inputCount()];
    const char* target=board_->socket[completed_%inputCount()].net;
    const bool expected=(contact.expected>>(completed_%inputCount()))&1;
    Result value=sample==ITestFixture::Sample::Error?Result::Error:
        connected?(expected?Result::Connected:Result::Unexpected):(expected?Result::Open:Result::Isolated);
    if(sample==ITestFixture::Sample::Ready&&!expected) {
        if(forwardRule(policy_,contact.net,target)>=0)value=connected?Result::DirectedPending:Result::Open;
        else if(reverseRule(policy_,contact.net,target)>=0)value=connected?Result::Unexpected:Result::DirectedBlocked;
    }
    results_[step_][completed_]=value;
    ++completed_; completedByConnector_[step_] = completed_; lastSample_ = now;
    if (completed_ == connector()->count * inputCount()) finish();
    else pathReady_ = select();
}
namespace { bool failure(Result r){return r==Result::Unexpected||r==Result::Open||r==Result::Error;} }
bool TestRunner::positivePath(size_t c,size_t p) const {
    const uint64_t expected=board_->connectors[c].contacts[p].expected;
    if(!expected)return false;
    for(size_t i=0;i<inputCount();++i)if(((expected>>i)&1)&&results_[c][p*inputCount()+i]!=Result::Connected)return false;
    return true;
}
Result TestRunner::evaluated(size_t c,size_t cell) const {
    const auto raw=results_[c][cell];
    if(raw!=Result::DirectedPending||!policy_||!board_)return raw;
    const size_t p=cell/inputCount(),i=cell%inputCount();
    const int rule=forwardRule(policy_,board_->connectors[c].contacts[p].net,board_->socket[i].net);
    if(rule<0)return Result::Unexpected;
    if(reverseEvidence_[rule]==2)return Result::Unexpected;
    return reverseEvidence_[rule]==1&&positivePath(c,p)?Result::DirectedConnected:Result::DirectedPending;
}
void TestRunner::refreshVerdicts() {
    std::fill_n(reverseEvidence_,MaxDirectedRules,0);
    if(policy_)for(size_t rule=0;rule<policy_->count;++rule) {
        bool blocked=false,conducting=false;
        for(size_t c=0;c<board_->count;++c) {
            const auto& connector=board_->connectors[c];
            if(completedByConnector_[c]!=connector.count*inputCount())continue;
            for(size_t p=0;p<connector.count;++p) {
                if(strcmp(connector.contacts[p].net,policy_->rules[rule].reverseSource))continue;
                bool seen=false,allBlocked=true;
                for(size_t i=0;i<inputCount();++i)if(!strcmp(board_->socket[i].net,policy_->rules[rule].from)) {
                    seen=true;const auto r=results_[c][p*inputCount()+i];
                    conducting|=r==Result::Unexpected;
                    allBlocked&=r==Result::DirectedBlocked;
                }
                blocked|=seen&&allBlocked&&positivePath(c,p);
            }
        }
        reverseEvidence_[rule]=conducting?2:blocked?1:0;
    }
    for(size_t c=0;c<board_->count;++c) {
        const auto& connector=board_->connectors[c];
        if(completedByConnector_[c]!=connector.count*inputCount()) {
            verdicts_[c]=Verdict::Untested;std::fill_n(contactVerdicts_[c],MaxContacts,Verdict::Untested);continue;
        }
        bool failed=false,incomplete=false;
        for(size_t p=0;p<connector.count;++p) {
            bool rowFailed=false,rowIncomplete=!connector.contacts[p].expected;
            for(size_t i=0;i<inputCount();++i) {
                const auto r=evaluated(c,p*inputCount()+i);
                rowFailed|=failure(r);rowIncomplete|=r==Result::DirectedPending;
            }
            contactVerdicts_[c][p]=rowFailed?Verdict::Failed:rowIncomplete?Verdict::Incomplete:Verdict::Passed;
            failed|=rowFailed;incomplete|=rowIncomplete;
        }
        verdicts_[c]=failed?Verdict::Failed:incomplete?Verdict::Incomplete:Verdict::Passed;
    }
}
void TestRunner::finish() {
    fixture_.release();phase_=Phase::Review;refreshVerdicts();++generation_;
}
bool TestRunner::selectConnector(size_t index) {
    if (!board_ || index >= board_->count || phase_ == Phase::Running) return false;
    fixture_.release(); step_ = index; completed_ = completedByConnector_[index];
    phase_ = verdicts_[index] == Verdict::Untested ? Phase::AwaitingConnection : Phase::Review;
    ++generation_; return true;
}
bool TestRunner::next() {
    if (phase_ != Phase::Review) return false;
    // Continue with the next untested connector, wrapping after the last one.
    for (size_t offset = 1; offset <= board_->count; ++offset) {
        const size_t candidate = (step_ + offset) % board_->count;
        if (verdicts_[candidate] == Verdict::Untested) return selectConnector(candidate);
    }
    fixture_.release(); step_ = board_->count; completed_ = 0;
    phase_ = Phase::Complete; ++generation_; return true;
}
void TestRunner::abort() { fixture_.release(); phase_ = Phase::Aborted; if(board_)refreshVerdicts();++generation_; }
Verdict TestRunner::contactVerdict(size_t step, size_t contact) const {
    return step < MaxConnectors && contact < MaxContacts ? contactVerdicts_[step][contact] : Verdict::Failed;
}
Verdict TestRunner::overall() const {
    if (!board_) return Verdict::Untested;
    bool incomplete = phase_ == Phase::Aborted;
    for (size_t i = 0; i < board_->count; ++i) {
        if (verdicts_[i] == Verdict::Failed) return Verdict::Failed;
        incomplete |= verdicts_[i] != Verdict::Passed;
    }
    return incomplete ? Verdict::Incomplete : Verdict::Passed;
}
} }
