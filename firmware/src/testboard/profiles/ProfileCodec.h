// EnvNode: see repository LICENSE.
#pragma once
#include "testing/TestModel.h"
#include "CompactCborCodec.h"
#include <Arduino.h>
namespace EnvNode { namespace Testing {
constexpr size_t MaximumProfileBytes = 20000, MaximumCatalogBytes = 8192;
constexpr size_t MaximumRemoteProfiles = 8, MaximumProfileContacts = 160;
constexpr const char* ProfileBaseUrl = "https://docs.envnode.de/assets/testboard/v1/";
struct RemoteProfileEntry {
    char id[96], name[96], identity[24], pcb[24], filename[176], sha256[65];
    size_t bytes;
};
struct RemoteCatalog { size_t count = 0; RemoteProfileEntry entries[MaximumRemoteProfiles]; };
struct OwnedProfile {
    BoardDefinition board{};
    SocketContact inputs[MaxInputs];
    Connector connectors[MaxConnectors];
    Contact contacts[MaximumProfileContacts];
    const char* labels[MaxConnectors];
    const char* sha256 = nullptr;
    const char* source = nullptr;
    const char* sourceCommit = nullptr;
    const char* image = nullptr;
    char contentSha256[65] = {};
    const char* referencePaths[2][4] = {}; // connector, diode, anode, cathode
    uint8_t referencePins[2] = {};
    double bounds[4];
    char text[8192];
    size_t textUsed = 0;
};
bool safeProfileFilename(const char* filename);
bool decodeCatalog(const uint8_t* data, size_t size, RemoteCatalog& output);
bool decodeProfile(const uint8_t* data, size_t size, const RemoteProfileEntry& entry, OwnedProfile& output);
String profileJson(const OwnedProfile& profile);
} }
