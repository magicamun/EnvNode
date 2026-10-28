// EnvNode: see repository LICENSE.
#pragma once
#include <memory>
#include "ProfileCodec.h"
#include "testing/TestRunner.h"
namespace EnvNode { namespace Testing {
class IProfileTransport {
public:
    virtual ~IProfileTransport() = default;
    // HTTPS transport must verify the requested SHA-256 when supplied.
    virtual bool fetch(const char* filename, size_t maximum, const char* sha256,
        std::unique_ptr<uint8_t[]>& output, size_t& size) = 0;
};
class ProfileRepository {
public:
    explicit ProfileRepository(IProfileTransport& transport) : transport_(transport) {}
    void requestRefresh() { pending_=true; }
    void service(bool networkReady, bool testRunning);
    uint32_t revision() const { return revision_; }
    const char* status() const { return status_; }
    size_t count() const;
    bool entryAt(size_t index, RemoteProfileEntry& entry, bool& remote) const;
    bool load(const char* id, TestRunner& runner);
    const OwnedProfile* prepare(const char* id, bool allowed);
    const OwnedProfile* active(const char* id) const;
private:
    const RemoteProfileEntry* remoteEntry(const char* id) const;
    IProfileTransport& transport_;
    RemoteCatalog catalog_{};
    std::unique_ptr<OwnedProfile> active_, prepared_;
    bool pending_=true;
    uint32_t revision_=0;
    const char* status_="waiting";
};
} }
