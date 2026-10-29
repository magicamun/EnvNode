// EnvNode: see repository LICENSE.
#pragma once
#include "HardwareProfileRepository.h"
#include "Logger.h"
namespace EnvNode {
class HardwareProfileHttpsTransport : public IHardwareProfileTransport {
public:
    explicit HardwareProfileHttpsTransport(ILogger& logger) : logger_(logger) {}
    bool fetch(const char*,size_t,const char*,std::unique_ptr<uint8_t[]>&,size_t&) override;
private:
    ILogger& logger_;
};
}
