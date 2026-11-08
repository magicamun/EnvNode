// EnvNode: see repository LICENSE.
#pragma once
#include "../testing/GuidedSelfTest.h"
namespace EnvNode {
// One electrical implementation shared by manual diagnosis, self-test and DUT tests.
// Construction does not touch GPIOs; callers must arbitrate ownership.
class Esp32FixtureIo : public Testing::IDiagnosticIo {
public:
    explicit Esp32FixtureIo(Testing::FixtureSelection* selection=nullptr):selection_(selection) {}
    void selectDiagnostic(const Testing::DiagnosticPath& path) override;
    uint16_t sampleDiagnostic() override;
    void releaseDiagnostic() override;
private:
    const Testing::FixtureDefinition& definition() const {return selection_?selection_->definition():Testing::fixtureRevision01();}
    Testing::FixtureSelection* selection_;
    bool configured_ = false;
};
}
