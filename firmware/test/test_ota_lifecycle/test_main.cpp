// EnvNode: see repository LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
#include <unity.h>
#include "OTAService.h"
#include "RuntimeManager.h"
#include <Update.h>
using namespace EnvNode;
UpdateClass Update;
HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t n) { return n; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}
class TestLogger : public ILogger {
public:
    void begin(unsigned long) override {}
    void println(const char*) override {}
    void printf(const char*, ...) override {}
};
class Restart : public IDeviceRestart {
public:
    bool pending = false;
    int requests = 0, executions = 0;
    void requestDeviceRestart() override { ++requests; pending = true; }
    bool deviceRestartPending() const override { return pending; }
    void performDeviceRestart() override { ++executions; }
};
class Driver : public IDeviceRestartDriver {
public:
    int calls = 0;
    void restart() override { ++calls; }
};
void setUp() { Update = UpdateClass{}; }
void tearDown() {}
void test_success_stages_known_size_and_requests_but_does_not_execute_restart() {
    TestLogger logger; Restart restart; OTAService ota(logger, restart); uint8_t data[4] = {};
    TEST_ASSERT_EQUAL(static_cast<int>(OTAState::Idle), static_cast<int>(ota.state()));
    TEST_ASSERT_TRUE(ota.beginUpload(4)); TEST_ASSERT_TRUE(ota.busy()); TEST_ASSERT_EQUAL(4, Update.requestedSize);
    TEST_ASSERT_TRUE(ota.writeChunk(data, 2)); TEST_ASSERT_EQUAL(50, ota.progressPercent());
    TEST_ASSERT_TRUE(ota.writeChunk(data, 2)); TEST_ASSERT_TRUE(ota.finishUpload());
    TEST_ASSERT_TRUE(ota.firmwareStaged()); TEST_ASSERT_FALSE(ota.busy()); TEST_ASSERT_TRUE(ota.restartRequired());
    TEST_ASSERT_EQUAL(100, ota.progressPercent()); TEST_ASSERT_EQUAL(1, restart.requests);
    TEST_ASSERT_EQUAL(0, restart.executions); TEST_ASSERT_FALSE(Update.endEvenIfRemaining);
    TEST_ASSERT_FALSE(ota.beginUpload(4)); TEST_ASSERT_FALSE(ota.finishUpload());
    ota.abortUpload("late abort"); ota.rejectUpload("late rejection"); TEST_ASSERT_TRUE(ota.firmwareStaged());
    restart.pending = false; TEST_ASSERT_FALSE(ota.restartRequired());
}
void test_unknown_size_uses_explicit_unknown_finalize_mode() {
    TestLogger logger; Restart restart; OTAService ota(logger, restart); uint8_t data[3] = {};
    TEST_ASSERT_TRUE(ota.beginUpload()); TEST_ASSERT_FALSE(ota.totalBytesKnown());
    TEST_ASSERT_EQUAL(UPDATE_SIZE_UNKNOWN, Update.requestedSize);
    TEST_ASSERT_TRUE(ota.writeChunk(data, 3)); TEST_ASSERT_TRUE(ota.finishUpload());
    TEST_ASSERT_TRUE(Update.endEvenIfRemaining); TEST_ASSERT_EQUAL(1, restart.requests);
    TEST_ASSERT_EQUAL(0, ota.progressPercent());
}
void test_empty_upload_is_aborted_without_restart() {
    TestLogger logger; Restart restart; OTAService ota(logger, restart);
    TEST_ASSERT_TRUE(ota.beginUpload(4)); TEST_ASSERT_FALSE(ota.finishUpload());
    TEST_ASSERT_EQUAL(1, Update.aborts); TEST_ASSERT_EQUAL(0, Update.ends);
    TEST_ASSERT_EQUAL(0, restart.requests); TEST_ASSERT_FALSE(ota.lastError().isEmpty());
}
void test_truncated_known_size_is_not_finalized() {
    TestLogger logger; Restart restart; OTAService ota(logger, restart); uint8_t data[2] = {};
    TEST_ASSERT_TRUE(ota.beginUpload(4)); TEST_ASSERT_TRUE(ota.writeChunk(data, 2));
    TEST_ASSERT_FALSE(ota.finishUpload()); TEST_ASSERT_EQUAL(1, Update.aborts);
    TEST_ASSERT_EQUAL(0, Update.ends); TEST_ASSERT_EQUAL(0, restart.requests);
}
void test_short_write_counts_only_written_bytes_and_aborts() {
    TestLogger logger; Restart restart; OTAService ota(logger, restart); uint8_t data[4] = {};
    TEST_ASSERT_TRUE(ota.beginUpload(4)); Update.writeLimit = 2;
    TEST_ASSERT_FALSE(ota.writeChunk(data, 4)); TEST_ASSERT_EQUAL(2, ota.bytesReceived());
    TEST_ASSERT_EQUAL(1, Update.aborts); TEST_ASSERT_EQUAL(0, restart.requests);
    TEST_ASSERT_EQUAL_STRING("driver failure", ota.lastError().c_str());
}
void test_initialization_failure_can_retry_without_pending_restart() {
    TestLogger logger; Restart restart; OTAService ota(logger, restart);
    Update.beginOk = false; TEST_ASSERT_FALSE(ota.beginUpload(4)); TEST_ASSERT_FALSE(ota.busy());
    TEST_ASSERT_EQUAL(0, Update.aborts); TEST_ASSERT_EQUAL(0, restart.requests);
    Update.beginOk = true; TEST_ASSERT_TRUE(ota.beginUpload(4)); TEST_ASSERT_TRUE(ota.lastError().isEmpty());
}
void test_finalization_failure_or_driver_error_never_stages() {
    TestLogger logger; Restart restart; OTAService ota(logger, restart); uint8_t data[4] = {};
    TEST_ASSERT_TRUE(ota.beginUpload(4)); TEST_ASSERT_TRUE(ota.writeChunk(data, 4));
    Update.endOk = false; TEST_ASSERT_FALSE(ota.finishUpload()); TEST_ASSERT_FALSE(ota.firmwareStaged());
    TEST_ASSERT_EQUAL(0, restart.requests);
    Update.endOk = true; TEST_ASSERT_TRUE(ota.beginUpload(4)); TEST_ASSERT_TRUE(ota.writeChunk(data, 4));
    Update.finalizeError = true; TEST_ASSERT_FALSE(ota.finishUpload()); TEST_ASSERT_EQUAL(0, restart.requests);
}
void test_abort_and_reject_preserve_active_upload_rules() {
    TestLogger logger; Restart restart; OTAService ota(logger, restart);
    ota.rejectUpload("bad file"); TEST_ASSERT_EQUAL_STRING("bad file", ota.lastError().c_str());
    TEST_ASSERT_TRUE(ota.beginUpload(4)); ota.rejectUpload("do not replace active upload");
    TEST_ASSERT_TRUE(ota.busy()); TEST_ASSERT_TRUE(ota.lastError().isEmpty());
    ota.abortUpload("cancelled"); TEST_ASSERT_EQUAL(1, Update.aborts);
    TEST_ASSERT_EQUAL_STRING("cancelled", ota.lastError().c_str()); TEST_ASSERT_EQUAL(0, restart.requests);
    ota.abortUpload("again"); TEST_ASSERT_EQUAL(1, Update.aborts);
}
void test_runtime_adapter_preserves_priority_and_explicit_execution_gate() {
    TestLogger logger; Driver driver; RuntimeManager runtime(logger, nullptr, nullptr, nullptr, &driver);
    IDeviceRestart& restart = runtime;
    runtime.request(RuntimeAction::RestartWiFi); restart.performDeviceRestart(); TEST_ASSERT_EQUAL(0, driver.calls);
    restart.requestDeviceRestart(); TEST_ASSERT_TRUE(restart.deviceRestartPending());
    runtime.request(RuntimeAction::RestartMqtt); runtime.service(); TEST_ASSERT_EQUAL(0, driver.calls);
    TEST_ASSERT_EQUAL(static_cast<int>(RuntimeAction::RestartDevice), static_cast<int>(runtime.pendingAction()));
    restart.performDeviceRestart(); TEST_ASSERT_EQUAL(1, driver.calls);
    runtime.clearPending(); TEST_ASSERT_FALSE(restart.deviceRestartPending());
    restart.performDeviceRestart(); TEST_ASSERT_EQUAL(1, driver.calls);
}
void test_ota_with_envnode_adapter_does_not_restart_during_staging() {
    TestLogger logger; Driver driver; RuntimeManager runtime(logger, nullptr, nullptr, nullptr, &driver);
    OTAService ota(logger, runtime); uint8_t data[4] = {};
    TEST_ASSERT_TRUE(ota.beginUpload(4)); TEST_ASSERT_TRUE(ota.writeChunk(data, 4)); TEST_ASSERT_TRUE(ota.finishUpload());
    TEST_ASSERT_TRUE(runtime.deviceRestartPending()); TEST_ASSERT_EQUAL(0, driver.calls);
    runtime.performPendingRestart(); TEST_ASSERT_EQUAL(1, driver.calls);
}
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_success_stages_known_size_and_requests_but_does_not_execute_restart);
    RUN_TEST(test_unknown_size_uses_explicit_unknown_finalize_mode);
    RUN_TEST(test_empty_upload_is_aborted_without_restart);
    RUN_TEST(test_truncated_known_size_is_not_finalized);
    RUN_TEST(test_short_write_counts_only_written_bytes_and_aborts);
    RUN_TEST(test_initialization_failure_can_retry_without_pending_restart);
    RUN_TEST(test_finalization_failure_or_driver_error_never_stages);
    RUN_TEST(test_abort_and_reject_preserve_active_upload_rules);
    RUN_TEST(test_runtime_adapter_preserves_priority_and_explicit_execution_gate);
    RUN_TEST(test_ota_with_envnode_adapter_does_not_restart_during_staging);
    return UNITY_END();
}
