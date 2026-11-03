// EnvNode: see repository LICENSE.
#include <unity.h>
#include <string>
#include <algorithm>
#include <cstring>
#include "SdCardDiagnostic.h"
#include "RecentLogStore.h"
#include "StructuredLogger.h"
#include "ILogTimeProvider.h"
#include "ILogEntrySink.h"
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}
using namespace EnvNode;
namespace {
struct Log : ILogger {
    std::string messages;
    void begin(unsigned long) override {}
    void println(const char* text) override { messages += text; messages += '\n'; }
    void printf(const char*, ...) override {}
};
struct Device : ISdCardDiagnosticDevice {
    bool present = true, mounted = true;
    unsigned mounts = 0, reads = 0, releases = 0, writeTests = 0;
    bool writePassed = true;
    SdCardDiagnosticInfo info;
    Device() { info.type="SDHC/SDXC"; info.cardBytes=32000000000ULL;
        info.filesystemBytes=31000000000ULL; info.usedBytes=1000000000ULL; info.rootReadable=true; }
    bool cardDetected() override { return present; }
    bool mount() override { ++mounts; return mounted; }
    SdCardDiagnosticInfo information() override { ++reads; return info; }
    void release() override { ++releases; }
    bool writeReadTest(ILogger&) override { ++writeTests; return writePassed; }
};
struct TestFile : ISdTestFile {
    std::string data, createdPath;
    size_t position = 0;
    unsigned attempts = 0, removals = 0, closes = 0;
    bool corrupt = false, writeFails = false, syncFails = false, reopenFails = false, removeFails = false;
    bool allExist = false, partial = false;
    SdTestFileCreate createExclusive(const char* path) override {
        ++attempts;
        if (allExist || attempts == 1) return SdTestFileCreate::Exists;
        createdPath = path; data.clear(); return SdTestFileCreate::Created;
    }
    int write(const uint8_t* bytes, size_t size) override {
        if (writeFails) return -1;
        if (partial && size > 7) size = 7;
        data.append(reinterpret_cast<const char*>(bytes), size); return size;
    }
    bool sync() override { return !syncFails; }
    bool close() override { ++closes; return true; }
    bool openRead(const char*) override {
        position=0;
        if (corrupt && !data.empty()) data[100] ^= 1;
        return !reopenFails;
    }
    int read(uint8_t* bytes, size_t size) override {
        size = std::min(size, data.size()-position);
        if (partial && size > 11) size = 11;
        memcpy(bytes, data.data()+position, size); position += size; return size;
    }
    bool remove(const char* path) override {
        ++removals;
        TEST_ASSERT_EQUAL_STRING(createdPath.c_str(), path);
        return !removeFails;
    }
};
void test_write_read_skips_existing_file_and_verifies_partial_io() {
    TestFile file; Log log; file.partial=true;
    TEST_ASSERT_TRUE(runSdWriteReadTest(file, log));
    TEST_ASSERT_EQUAL_STRING("/envnode-sd-test-01.bin", file.createdPath.c_str());
    TEST_ASSERT_EQUAL_UINT(4096, file.data.size());
    TEST_ASSERT_EQUAL_UINT(2, file.closes); TEST_ASSERT_EQUAL_UINT(1, file.removals);
}
void test_existing_files_are_never_removed() {
    TestFile file; Log log; file.allExist=true;
    TEST_ASSERT_FALSE(runSdWriteReadTest(file, log));
    TEST_ASSERT_EQUAL_UINT(32, file.attempts); TEST_ASSERT_EQUAL_UINT(0, file.removals);
}
void test_write_read_failures_cleanup_and_never_report_pass() {
    for (unsigned fault=0; fault<5; ++fault) {
        TestFile file; Log log;
        file.writeFails=fault==0; file.syncFails=fault==1; file.reopenFails=fault==2;
        file.corrupt=fault==3; file.removeFails=fault==4;
        TEST_ASSERT_FALSE(runSdWriteReadTest(file, log));
        TEST_ASSERT_EQUAL_UINT(1, file.removals);
        TEST_ASSERT_EQUAL(std::string::npos, log.messages.find("PASS"));
    }
}
void test_device_write_failure_unmounts_without_overall_pass() {
    Device device; Log log; device.writePassed=false;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SdCardDiagnosticResult::WriteReadFailed), static_cast<int>(runSdCardDiagnostic(device,log)));
    TEST_ASSERT_EQUAL_UINT(1, device.releases);
    TEST_ASSERT_EQUAL(std::string::npos, log.messages.find("PASS"));
}
void test_disabled_log_retention_still_delivers_to_sink() {
    struct Clock : ILogTimeProvider {
        uint32_t monotonicMs() const override { return 42; }
        bool wallClockEpochSeconds(int64_t&) const override { return false; }
    } clock;
    struct Sink : ILogEntrySink {
        unsigned writes = 0;
        void begin(unsigned long) override {}
        void write(const LogEntry&) override { ++writes; }
    } sink;
    RecentLogStore store(0);
    StructuredLogger logger(store, clock, sink);
    logger.info("SD diagnostic: retention unavailable");
    TEST_ASSERT_EQUAL_UINT(1, sink.writes);
    TEST_ASSERT_EQUAL_UINT(0, store.capacity());
    TEST_ASSERT_EQUAL_UINT(0, store.count());
    LogEntry entry;
    TEST_ASSERT_FALSE(store.copyEntry(0, entry));
}
void test_small_log_buffer_wraps_and_clears() {
    RecentLogStore store(2);
    LogEntry entry;
    for (unsigned sequence = 1; sequence <= 3; ++sequence) {
        entry.sequence = sequence; store.append(entry);
    }
    TEST_ASSERT_EQUAL_UINT(2, store.count());
    TEST_ASSERT_TRUE(store.copyEntry(0, entry));
    TEST_ASSERT_EQUAL_UINT(2, entry.sequence);
    TEST_ASSERT_TRUE(store.copyEntry(1, entry));
    TEST_ASSERT_EQUAL_UINT(3, entry.sequence);
    store.clear();
    TEST_ASSERT_EQUAL_UINT(0, store.count());
    TEST_ASSERT_EQUAL_UINT(2, store.capacity());
}
void test_absent_card_never_initializes_spi() {
    Device d; Log l; d.present=false;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SdCardDiagnosticResult::NoCard), static_cast<int>(runSdCardDiagnostic(d,l)));
    TEST_ASSERT_EQUAL_UINT(0,d.mounts); TEST_ASSERT_EQUAL_UINT(0,d.reads); TEST_ASSERT_EQUAL_UINT(0,d.writeTests);
    TEST_ASSERT_NOT_EQUAL(std::string::npos,l.messages.find("CD=LOW"));
}
void test_mount_failure_releases_resources_without_reading() {
    Device d; Log l; d.mounted=false;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SdCardDiagnosticResult::MountFailed), static_cast<int>(runSdCardDiagnostic(d,l)));
    TEST_ASSERT_EQUAL_UINT(1,d.releases); TEST_ASSERT_EQUAL_UINT(0,d.reads); TEST_ASSERT_EQUAL_UINT(0,d.writeTests);
    TEST_ASSERT_EQUAL(std::string::npos,l.messages.find("PASS"));
}
void test_valid_card_reports_64_bit_sizes_and_unmounts() {
    Device d; Log l;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SdCardDiagnosticResult::Passed), static_cast<int>(runSdCardDiagnostic(d,l)));
    TEST_ASSERT_EQUAL_UINT(1,d.releases); TEST_ASSERT_EQUAL_UINT(1,d.reads); TEST_ASSERT_EQUAL_UINT(1,d.writeTests);
    TEST_ASSERT_NOT_EQUAL(std::string::npos,l.messages.find("32000000000"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos,l.messages.find("PASS"));
}
void test_unreadable_root_is_failure_and_releases_resources() {
    Device d; Log l; d.info.rootReadable=false;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SdCardDiagnosticResult::ReadFailed), static_cast<int>(runSdCardDiagnostic(d,l)));
    TEST_ASSERT_EQUAL_UINT(1,d.releases); TEST_ASSERT_EQUAL(std::string::npos,l.messages.find("PASS"));
}
void test_invalid_capacity_or_usage_never_reports_pass() {
    Device d; Log l; d.info.filesystemBytes=0;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SdCardDiagnosticResult::ReadFailed), static_cast<int>(runSdCardDiagnostic(d,l)));
    d.info.filesystemBytes=100; d.info.usedBytes=101;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SdCardDiagnosticResult::ReadFailed), static_cast<int>(runSdCardDiagnostic(d,l)));
    TEST_ASSERT_EQUAL_UINT(2,d.releases);
}
}
void setUp() {} void tearDown() {}
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_write_read_skips_existing_file_and_verifies_partial_io);
    RUN_TEST(test_existing_files_are_never_removed);
    RUN_TEST(test_write_read_failures_cleanup_and_never_report_pass);
    RUN_TEST(test_device_write_failure_unmounts_without_overall_pass);
    RUN_TEST(test_disabled_log_retention_still_delivers_to_sink);
    RUN_TEST(test_small_log_buffer_wraps_and_clears);
    RUN_TEST(test_absent_card_never_initializes_spi);
    RUN_TEST(test_mount_failure_releases_resources_without_reading);
    RUN_TEST(test_valid_card_reports_64_bit_sizes_and_unmounts);
    RUN_TEST(test_unreadable_root_is_failure_and_releases_resources);
    RUN_TEST(test_invalid_capacity_or_usage_never_reports_pass);
    return UNITY_END();
}
