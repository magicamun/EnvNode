// EnvNode: see repository LICENSE.
#pragma once
#include "../testing/GuidedSelfTest.h"
namespace EnvNode {
// One electrical implementation shared by manual diagnosis, self-test and DUT tests.
// Construction does not touch GPIOs; callers must arbitrate ownership.
class Esp32FixtureIo : public Testing::IDiagnosticIo {
public:
    void selectDiagnostic(const Testing::DiagnosticPath& path) override;
    uint16_t sampleDiagnostic() override;
    void releaseDiagnostic() override;
private:
    bool configured_ = false;
};
}
