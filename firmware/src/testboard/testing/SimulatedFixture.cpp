// EnvNode: see repository LICENSE.
#include "SimulatedFixture.h"
#include "FixtureRouting.h"
namespace EnvNode { namespace Testing {
void SimulatedFixture::connect(const BoardDefinition& board, size_t connector, SimulatedFault fault) {
    release(); inputCount_ = board.inputCount; connector_ = connector < board.count ? &board.connectors[connector] : nullptr; fault_ = fault;
}
bool SimulatedFixture::select(Pigtail pigtail, uint8_t contact, uint8_t socket) {
    selected_ = connector_ && connector_->pigtail == pigtail && contact && contact <= connector_->count && socket && socket <= inputCount_;
    MuxAddress source{}, destination{};
    selected_ = selected_ && resolveFixturePath(pigtail, contact, socket, source, destination);
    contact_ = contact; socket_ = socket; return selected_;
}
ITestFixture::Sample SimulatedFixture::read(bool& connected) {
    if (!selected_) return Sample::Error;
    const uint64_t mask = connector_->contacts[contact_ - 1].expected;
    connected = (mask >> (socket_ - 1)) & 1;
    // Deterministic faults on contact 1, separate from the runner's comparison.
    if (contact_ == 1 && fault_ == SimulatedFault::OpenContact) connected = false;
    if (contact_ == 1 && fault_ == SimulatedFault::ExtraConnection) {
        for (size_t i = 0; i < inputCount_; ++i) if (!((mask >> i) & 1)) {
            if (socket_ == i + 1) connected = true;
            break;
        }
    }
    return Sample::Ready;
}
} }
