// EnvNode: see repository LICENSE.
#pragma once
#include "ProfileRepository.h"
#include "Logger.h"
namespace EnvNode { namespace Testing {
class HttpsProfileTransport : public IProfileTransport {
public:
    explicit HttpsProfileTransport(ILogger& logger) : logger_(logger) {}
    bool fetch(const char* filename, size_t maximum, const char* sha256,
        std::unique_ptr<uint8_t[]>& output, size_t& size) override;
private:
    ILogger& logger_;
};
} }
