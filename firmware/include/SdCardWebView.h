// EnvNode: see repository LICENSE.
#pragma once
#include <Arduino.h>
#include "EnvNode/Locale.h"
#include "SdCardDiagnostic.h"
#include "SdFileTransfer.h"
namespace EnvNode {
enum class SdDirectoryStatus { Disabled, Ready, NoCard, Conflict, MountFailed, NotFound, ReadFailed, InvalidPath };
constexpr size_t SdDirectoryPageSize = 32;
constexpr unsigned SdDirectoryMaxOffset = 1024;
struct SdDirectoryEntry { String name; uint64_t bytes = 0; bool directory = false; };
struct SdDirectorySnapshot {
    SdDirectoryStatus status = SdDirectoryStatus::Disabled;
    String path = "/";
    SdCardDiagnosticInfo info;
    SdDirectoryEntry entries[SdDirectoryPageSize];
    size_t count = 0;
    unsigned offset = 0;
    bool more = false;
    bool limited = false;
};
String renderSdFormatConfirmation(const SdFileStamp& card,const String& token,Locale locale);
String sdDownloadDisposition(const String& path);
String sdEncodeComponent(const String& value);
String sdParentPath(const String& path);
String renderSdDeleteConfirmation(const String& path,const SdFileStamp& stamp,const String& token,Locale locale);
bool validSdDirectoryPath(const String& path);
String sdDirectoryUrl(const String& path, unsigned offset = 0);
String renderSdDirectory(const SdDirectorySnapshot& snapshot, Locale locale);
}
