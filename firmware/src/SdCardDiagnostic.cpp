// EnvNode: see repository LICENSE.
#include "SdCardDiagnostic.h"
#include <cstdio>
#include <cstring>
namespace EnvNode {
bool runSdWriteReadTest(ISdTestFile& file, ILogger& logger) {
    constexpr size_t TestBytes = 4096;
    constexpr size_t ChunkBytes = 256;
    char path[40] = {};
    bool created = false;
    for (unsigned index = 0; index < 32; ++index) {
        snprintf(path, sizeof(path), "/envnode-sd-test-%02u.bin", index);
        const SdTestFileCreate result = file.createExclusive(path);
        if (result == SdTestFileCreate::Exists) continue;
        if (result == SdTestFileCreate::Created) created = true;
        break;
    }
    if (!created) {
        logger.warn("SD write/read: cannot create exclusive test file; existing files untouched");
        return false;
    }
    logger.infof("SD write/read: file=%s bytes=%u", path, static_cast<unsigned>(TestBytes));
    uint8_t expected[ChunkBytes];
    uint8_t actual[ChunkBytes];
    const auto fill = [&](size_t offset) {
        for (size_t i = 0; i < ChunkBytes; ++i) {
            const size_t position = offset + i;
            expected[i] = static_cast<uint8_t>((position * 37 + (position >> 8)) ^ 0xA5);
        }
    };
    bool ok = true;
    for (size_t offset = 0; ok && offset < TestBytes; offset += ChunkBytes) {
        fill(offset);
        size_t written = 0;
        while (ok && written < ChunkBytes) {
            const int count = file.write(expected + written, ChunkBytes - written);
            if (count <= 0 || static_cast<size_t>(count) > ChunkBytes - written) ok = false;
            else written += count;
        }
    }
    if (ok) ok = file.sync();
    const bool writeClosed = file.close();
    ok = ok && writeClosed;
    if (ok) {
        ok = file.openRead(path);
        if (ok) {
            for (size_t offset = 0; ok && offset < TestBytes; offset += ChunkBytes) {
                fill(offset);
                size_t received = 0;
                while (ok && received < ChunkBytes) {
                    const int count = file.read(actual + received, ChunkBytes - received);
                    if (count <= 0 || static_cast<size_t>(count) > ChunkBytes - received) ok = false;
                    else received += count;
                }
                if (ok && memcmp(expected, actual, ChunkBytes) != 0) ok = false;
            }
            if (ok) ok = file.read(actual, 1) == 0; // Exact file length, no trailing data.
            const bool readClosed = file.close();
            ok = ok && readClosed;
        }
    }
    // Only the path this invocation created exclusively may be removed.
    const bool removed = file.remove(path);
    if (!removed) logger.warnf("SD write/read: cleanup failed; own test file remains: %s", path);
    if (!ok) logger.warn("SD write/read: write, sync, reopen or byte comparison failed");
    if (ok && removed) logger.info("SD write/read: PASS; 4096 bytes verified; own test file removed");
    return ok && removed;
}

SdCardDiagnosticResult runSdCardDiagnostic(ISdCardDiagnosticDevice& device, ILogger& logger) {
    const bool present = device.cardDetected();
    logger.infof("SD diagnostic: CD=%s (HIGH=inserted, LOW=absent)", present ? "HIGH" : "LOW");
    if (!present) {
        logger.warn("SD diagnostic: no card detected; insert card and restart to retry");
        return SdCardDiagnosticResult::NoCard;
    }
    if (!device.mount()) {
        device.release();
        logger.warn("SD diagnostic: SPI initialization or FAT mount failed; check wiring, supply and card format; no format attempted");
        return SdCardDiagnosticResult::MountFailed;
    }
    const SdCardDiagnosticInfo info = device.information();
    logger.infof("SD diagnostic: type=%s card_bytes=%llu", info.type,
        static_cast<unsigned long long>(info.cardBytes));
    logger.infof("SD diagnostic: filesystem_bytes=%llu used_bytes=%llu root=%s",
        static_cast<unsigned long long>(info.filesystemBytes),
        static_cast<unsigned long long>(info.usedBytes), info.rootReadable ? "readable" : "unreadable");
    const bool passed = info.cardBytes > 0 && info.filesystemBytes > 0
        && info.filesystemBytes <= info.cardBytes
        && info.usedBytes <= info.filesystemBytes && info.rootReadable;
    if (!passed) {
        device.release();
        logger.warn("SD diagnostic: filesystem read check failed; resources released");
        return SdCardDiagnosticResult::ReadFailed;
    }
    const bool writePassed = device.writeReadTest(logger);
    device.release();
    if (!writePassed) return SdCardDiagnosticResult::WriteReadFailed;
    logger.info("SD diagnostic: PASS; read/write verified; filesystem unmounted");
    return SdCardDiagnosticResult::Passed;
}
}
