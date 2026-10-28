// EnvNode: see repository LICENSE.
#pragma once
#include "TestModel.h"
namespace EnvNode { namespace Testing {
enum class SimulatedFault : uint8_t { None, OpenContact, ExtraConnection };
class SimulatedFixture : public ITestFixture {
public:
    void connect(const BoardDefinition& board, size_t connector, SimulatedFault fault);
    bool select(Pigtail pigtail, uint8_t contact, uint8_t socket) override;
    Sample read(bool& connected) override;
    void release() override { selected_ = false; }
private:
    const Connector* connector_ = nullptr;
    size_t inputCount_ = 0;
    SimulatedFault fault_ = SimulatedFault::None;
    uint8_t contact_ = 0, socket_ = 0;
    bool selected_ = false;
};
} }
