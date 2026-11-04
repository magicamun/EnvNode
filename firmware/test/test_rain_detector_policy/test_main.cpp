// EnvNode-owned: see ../../../LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
#include <unity.h>
#include <Arduino.h>
#include <RainDetectorPolicy.h>
#include "RainFrequencyConversion.h"
#include "RainFrequencyAcquisition.h"
#include "RainDetectorSensor.h"
#include "RainDetectorController.h"
#include "ControllerRuntime.h"
#include "UnitConverter.h"
#include "MqttTopic.h"
using namespace EnvNode;
HardwareSerial Serial;
// Native firmware build links the existing logger/GPIO adapters as well.
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t count) { return count; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}
void setUp() {}
void tearDown() {}
void test_shared_policy_wetting_end_and_dry_are_independent() {
    RainDetectorPolicy policy;
    policy.enable();
    for (uint64_t t=250; t<=6250; t+=250) policy.update(t,34000,0);
    TEST_ASSERT_TRUE(policy.update(6500,30000,0).wetting);
    for (uint64_t t=6750;t<=72000;t+=250) policy.update(t,30000,100);
    TEST_ASSERT_TRUE(policy.result().rain == RainDetectorRainState::EndEstimated);
    TEST_ASSERT_TRUE(policy.result().heating);
    for (uint64_t t=72250;t<=83000;t+=250) policy.update(t,33000,100);
    TEST_ASSERT_TRUE(policy.result().dry);
    TEST_ASSERT_FALSE(policy.result().heating);
}
void test_signal_loss_aborts_cycle_but_preserves_arming() {
    RainDetectorPolicy policy;
    policy.enable();
    for (uint64_t t=250;t<=6250;t+=250) policy.update(t,34000,0);
    TEST_ASSERT_TRUE(policy.update(6500,30000,0).heating);
    TEST_ASSERT_FALSE(policy.update(6750,NAN,100).heating);
    TEST_ASSERT_TRUE(policy.enabled());
    TEST_ASSERT_TRUE(policy.result().rain == RainDetectorRainState::Unknown);
    for (uint64_t t=7000;t<=14000;t+=250) policy.update(t,30000,0);
    TEST_ASSERT_FALSE(policy.result().heating);
    TEST_ASSERT_TRUE(policy.update(14250,29000,0).heating);
}
void test_config_is_atomic_and_bounded() {
    RainDetectorPolicy policy;
    RainDetectorPolicyConfig config;
    config.endSeconds=29;
    TEST_ASSERT_FALSE(policy.configure(config));
    TEST_ASSERT_EQUAL_UINT(60,policy.config().endSeconds);
    config.endSeconds=300;
    TEST_ASSERT_TRUE(policy.configure(config));
    policy.enable();
    TEST_ASSERT_FALSE(policy.configure(config));
    policy.disable();
    TEST_ASSERT_FALSE(policy.result().heating);
}

void test_startup_above_threshold_confirms_dry_without_heat_or_rain() {
    RainDetectorPolicy policy; policy.enable();
    for(uint64_t t=250;t<=15000;t+=250) {
        const auto result=policy.update(t,32000,0);
        TEST_ASSERT_FALSE(result.heating); TEST_ASSERT_FALSE(result.wetting);
        TEST_ASSERT_TRUE(result.rain==RainDetectorRainState::Unknown);
        TEST_ASSERT_FALSE(result.dry);
    }
    TEST_ASSERT_TRUE(policy.update(15250,32000,0).dry);
    // A slow fall after completed startup must not create a second startup cycle.
    for(uint64_t t=15500;t<=35500;t+=250) policy.update(t,32000-(t-15250)/10.0f,0);
    TEST_ASSERT_FALSE(policy.result().heating);
}
void test_startup_below_threshold_drives_one_drying_cycle_without_rain() {
    RainDetectorPolicy policy; policy.enable();
    for(uint64_t t=250;t<5250;t+=250) TEST_ASSERT_FALSE(policy.update(t,30000,0).heating);
    const auto started=policy.update(5250,30000,0);
    TEST_ASSERT_TRUE(started.heating); TEST_ASSERT_FALSE(started.wetting);
    TEST_ASSERT_TRUE(started.rain==RainDetectorRainState::Unknown);
    for(uint64_t t=5500;t<=22000;t+=250) policy.update(t,33000,100);
    TEST_ASSERT_TRUE(policy.result().dry); TEST_ASSERT_FALSE(policy.result().heating);
    TEST_ASSERT_TRUE(policy.result().rain==RainDetectorRainState::Unknown);
}
void test_startup_waits_for_valid_signal_and_fault_does_not_restart_heat() {
    RainDetectorPolicy policy; policy.enable();
    for(uint64_t t=250;t<=10000;t+=250) TEST_ASSERT_FALSE(policy.update(t,NAN,0).heating);
    TEST_ASSERT_TRUE(policy.startupPending());
    for(uint64_t t=10250;t<=15250;t+=250) policy.update(t,30000,0);
    TEST_ASSERT_TRUE(policy.result().heating); TEST_ASSERT_FALSE(policy.startupPending());
    TEST_ASSERT_FALSE(policy.update(15500,NAN,100).heating);
    for(uint64_t t=15750;t<=23000;t+=250) policy.update(t,30000,0);
    TEST_ASSERT_FALSE(policy.result().heating);
    TEST_ASSERT_TRUE(policy.update(23250,29000,0).wetting);
    TEST_ASSERT_TRUE(policy.result().heating);
}
void test_startup_high_then_low_restarts_confirmation_with_full_heat() {
    RainDetectorPolicy policy; policy.enable();
    for(uint64_t t=250;t<=10000;t+=250) policy.update(t,32100,0);
    // Smaller than both wetting thresholds: threshold crossing alone during startup.
    const auto result=policy.update(10250,31999,0);
    TEST_ASSERT_TRUE(result.heating); TEST_ASSERT_FALSE(result.dry); TEST_ASSERT_FALSE(result.wetting);
    TEST_ASSERT_TRUE(result.rain==RainDetectorRainState::Unknown);
    policy.disable(); TEST_ASSERT_FALSE(policy.result().heating);
    policy.enable();
    for(uint64_t t=10500;t<=15750;t+=250) policy.update(t,30000,0);
    TEST_ASSERT_TRUE(policy.result().heating);
}
class TestFrequencyInput : public IRainFrequencyInput {
public:
    bool startOk = true;
    unsigned stopped = 0;
    RainFrequencyPoll next = RainFrequencyPoll::Pending;
    RainFrequencyWindow window;
    bool begin(uint8_t gpio) override { return startOk && gpio == 27; }
    RainFrequencyPoll poll(RainFrequencyWindow& value) override { value = window; return next; }
    void stop() override { ++stopped; }
};
class TestFrequencyObserver : public IRainFrequencyObserver {
public:
    unsigned count = 0;
    RainFrequencyWindow last;
    void onRainFrequency(const RainFrequencyWindow& window) override { ++count; last = window; }
};
void test_frequency_conversion_detects_zero_wrap_and_gate_errors() {
    auto good = rainFrequencyWindow(5000000000000ULL, 5000000250000ULL, 10000, false);
    TEST_ASSERT_TRUE(good.valid());
    TEST_ASSERT_FLOAT_WITHIN(0.01F,40000,good.frequencyHz);
    TEST_ASSERT_TRUE(rainFrequencyWindow(0,250000,0,false).fault == RainFrequencyFault::NoSignal);
    TEST_ASSERT_TRUE(rainFrequencyWindow(0,250000,100,true).fault == RainFrequencyFault::CounterOverflow);
    TEST_ASSERT_TRUE(rainFrequencyWindow(0,1600000,100,false).fault == RainFrequencyFault::InvalidWindow);
    TEST_ASSERT_FALSE(rainFrequencyWindow(100,100,10,false).valid());
    TEST_ASSERT_FALSE(rainFrequencyWindow(0,1000,10,false).valid());
}
void test_acquisition_delivers_every_window_and_fault_without_publication_poll() {
    TestFrequencyInput input;
    TestFrequencyObserver observer;
    RainFrequencyAcquisition acquisition(input,observer);
    TEST_ASSERT_TRUE(acquisition.begin(27));
    acquisition.service();
    TEST_ASSERT_EQUAL_UINT(0,observer.count);
    input.next=RainFrequencyPoll::Window;
    input.window=rainFrequencyWindow(0,250000,10000,false);
    acquisition.service(); acquisition.service();
    TEST_ASSERT_EQUAL_UINT(2,observer.count);
    input.window=rainFrequencyWindow(250000,500000,0,false);
    acquisition.service();
    TEST_ASSERT_FALSE(observer.last.valid());
    TEST_ASSERT_TRUE(acquisition.running()); // Missing signal may recover next gate.
    input.window=rainFrequencyWindow(500000,750000,9000,false);
    acquisition.service();
    TEST_ASSERT_TRUE(observer.last.valid());
    RainFrequencyWindow copy;
    TEST_ASSERT_TRUE(acquisition.latest(copy));
    TEST_ASSERT_EQUAL_UINT(9000,copy.pulseCount);
    input.next=RainFrequencyPoll::Failed;
    acquisition.service();
    TEST_ASSERT_FALSE(acquisition.running());
    TEST_ASSERT_TRUE(observer.last.fault == RainFrequencyFault::DriverError);
    unsigned count=observer.count;
    acquisition.service();
    TEST_ASSERT_EQUAL_UINT(count,observer.count);
    input.startOk=false;
    TEST_ASSERT_FALSE(acquisition.begin(27));
    TEST_ASSERT_FALSE(observer.last.valid());
    input.startOk=true; input.next=RainFrequencyPoll::Pending;
    TEST_ASSERT_TRUE(acquisition.begin(27));
    TEST_ASSERT_FALSE(acquisition.latest(copy));
}
class FrequencyTestClock : public IMonotonicClock {
public:
    uint32_t now = 0;
    uint32_t nowMs() const override { return now; }
};
class FrequencyMeasurementSink : public IMeasurementSink {
public:
    Measurement last;
    unsigned count = 0;
    void emit(const Measurement& m) override { last=m; ++count; }
};
void test_sensor_keeps_local_frequency_independent_of_sample_publication() {
    TestFrequencyInput input;
    FrequencyTestClock clock;
    FrequencyMeasurementSink sink;
    RainDetectorSensor sensor(1,27,input,clock);
    sensor.begin();
    input.next=RainFrequencyPoll::Window;
    input.window=rainFrequencyWindow(0,250000,10000,false);
    clock.now=250; sensor.service(sink);
    TEST_ASSERT_EQUAL_UINT(0,sink.count);
    RainFrequencySnapshot snapshot;
    TEST_ASSERT_TRUE(sensor.rainFrequencySnapshot(snapshot));
    TEST_ASSERT_EQUAL_UINT(1,snapshot.revision);
    TEST_ASSERT_TRUE(snapshot.window.valid());
    sensor.sample(sink);
    TEST_ASSERT_TRUE(sink.last.valid);
    TEST_ASSERT_TRUE(sink.last.type == MeasurementType::RainDetectorFrequency);
    TEST_ASSERT_TRUE(isMeasurementContentStructurallyValid(sink.last));
    input.window=rainFrequencyWindow(250000,500000,0,false);
    clock.now=500; sensor.service(sink);
    sensor.sample(sink); TEST_ASSERT_FALSE(sink.last.valid);
    TEST_ASSERT_TRUE(sensor.state()==SensorState::Degraded);
    input.window=rainFrequencyWindow(500000,750000,8000,false);
    clock.now=750; sensor.service(sink); sensor.sample(sink);
    TEST_ASSERT_TRUE(sink.last.valid);
    TEST_ASSERT_TRUE(sensor.state()==SensorState::Ready);
    clock.now=2500; sensor.sample(sink); TEST_ASSERT_FALSE(sink.last.valid);
    TEST_ASSERT_TRUE(isMeasurementContentStructurallyValid(sink.last));
    input.next=RainFrequencyPoll::Failed;
    sensor.service(sink); TEST_ASSERT_TRUE(sensor.state()==SensorState::Failed);
    input.next=RainFrequencyPoll::Pending;
    clock.now=3600; sensor.service(sink);
    sensor.sample(sink); TEST_ASSERT_FALSE(sink.last.valid);
}
void test_frequency_metadata_does_not_reuse_ratio_measurements() {
    TEST_ASSERT_TRUE(measurementTypeMetadata(MeasurementType::RainDetectorFrequency).canonicalUnit==PresentationUnit::Hertz);
    TEST_ASSERT_TRUE(measurementTypeMetadata(MeasurementType::RainDetectorLevel).canonicalUnit==PresentationUnit::Ratio);
    TEST_ASSERT_TRUE(measurementTypeFromStableId("rain_detector_frequency")==MeasurementType::RainDetectorFrequency);
    float hz=0;
    TEST_ASSERT_TRUE(UnitConverter::convert(MeasurementType::RainDetectorFrequency,32000,PresentationUnit::Hertz,hz));
    TEST_ASSERT_FLOAT_WITHIN(0.01F,32000,hz);
    TEST_ASSERT_TRUE(UnitConverter::convert(MeasurementType::RainDetectorFrequency,32000,PresentationUnit::Kilohertz,hz));
    TEST_ASSERT_FLOAT_WITHIN(0.001F,32,hz);
    PresentationUnit unit;
    TEST_ASSERT_TRUE(UnitConverter::parseStableKey("hertz",unit));
    TEST_ASSERT_TRUE(unit==PresentationUnit::Hertz);
    TEST_ASSERT_TRUE(UnitConverter::parseStableKey("kilohertz",unit));
    TEST_ASSERT_TRUE(unit==PresentationUnit::Kilohertz);
}
class RainTestLogger : public ILogger {
public:
    void begin(unsigned long) override {}
    void println(const char*) override {}
    void printf(const char*,...) override {}
};
class RainTestActor : public IOnOffActuator {
public:
    OnOffState output=OnOffState::Off;
    uint8_t percent=100;
    uint8_t outputPercent() const override { return output==OnOffState::On?percent:0; }
    ActuatorOperationResult begin() override { return ActuatorOperationResult::Completed; }
    ActuatorOperationResult shutdown() override { output=OnOffState::Off; return ActuatorOperationResult::Completed; }
    ActuatorOperationResult setState(OnOffState state) override { output=state; percent=state==OnOffState::On?100:0; return ActuatorOperationResult::Completed; }
    OnOffState state() const override { return output; }
    bool initialized() const override { return true; }
};
class RainTestActors : public IOnOffActuatorResolver {
public:
    RainTestActor actor;
    bool available=true;
    IOnOffActuator* onOffActuator(ActuatorId id) override { return available&&id==1?&actor:nullptr; }
};
class RainTestSource : public IRainFrequencyResolver {
public:
    RainFrequencySnapshot frame;
    bool available=true;
    unsigned gateRestarts=0;
    bool rainFrequencySnapshot(SensorId id,RainFrequencySnapshot& result) const override {
        if (!available||id!=1) return false;
        result=frame; return true;
    }
    void rainFrequencyHeaterChanged(SensorId) override { ++gateRestarts; }
};

void test_controller_initial_drying_has_no_wetting_and_target_recovery_stays_off() {
    RainDetectorControllerConfiguration cfg; cfg.sourceSensorId=1; cfg.targetActuatorId=1;
    RainTestSource source; RainTestActors actors; FrequencyTestClock clock; RainTestLogger logger;
    RainDetectorController controller(cfg,ModuleActuatorReference{},source,actors,clock,logger);
    TEST_ASSERT_TRUE(controller.begin()==ControllerOperationResult::Completed);
    const auto frame=[&](float hz) {
        clock.now+=250; ++source.frame.revision; source.frame.acceptedMs=clock.now;
        source.frame.window=rainFrequencyWindow((clock.now-250)*1000ULL,clock.now*1000ULL,
            static_cast<uint32_t>(hz/4),false);
        controller.service();
    };
    for(unsigned i=0;i<30;++i) frame(30000);
    TEST_ASSERT_TRUE(actors.actor.output==OnOffState::On);
    TEST_ASSERT_EQUAL_UINT32(0,controller.wettingCount());
    TEST_ASSERT_TRUE(controller.result().rain==RainDetectorRainState::Unknown);
    for(unsigned i=0;i<150;++i) frame(34000);
    TEST_ASSERT_TRUE(controller.result().dry);
    TEST_ASSERT_TRUE(actors.actor.output==OnOffState::Off);
    actors.available=false; controller.service();
    actors.available=true; controller.service();
    for(unsigned i=0;i<40;++i) frame(30000);
    TEST_ASSERT_TRUE(actors.actor.output==OnOffState::Off);
    frame(29000);
    TEST_ASSERT_TRUE(actors.actor.output==OnOffState::On);
    TEST_ASSERT_EQUAL_UINT32(1,controller.wettingCount());
}
void test_controller_operates_without_UTC_and_recovers_after_signal_failure() {
    RainDetectorControllerConfiguration cfg; cfg.sourceSensorId=1; cfg.targetActuatorId=1;
    RainTestSource source; RainTestActors actors; FrequencyTestClock clock; RainTestLogger logger;
    RainDetectorController controller(cfg,ModuleActuatorReference{},source,actors,clock,logger);
    TEST_ASSERT_TRUE(controller.begin()==ControllerOperationResult::Completed);
    const auto frame=[&](float hz) {
        clock.now+=250; ++source.frame.revision;
        source.frame.acceptedMs=clock.now;
        source.frame.window=rainFrequencyWindow((clock.now-250)*1000ULL,clock.now*1000ULL,
            static_cast<uint32_t>(hz/4),false);
        return controller.service();
    };
    for(unsigned i=0;i<25;++i)frame(34000);
    frame(30000); TEST_ASSERT_TRUE(actors.actor.output==OnOffState::On);
    frame(0); TEST_ASSERT_TRUE(actors.actor.output==OnOffState::Off);
    TEST_ASSERT_TRUE(controller.running());
    for(unsigned i=0;i<25;++i)frame(30000);
    TEST_ASSERT_TRUE(actors.actor.output==OnOffState::Off);
    frame(29000); TEST_ASSERT_TRUE(actors.actor.output==OnOffState::On);
    clock.now+=2000; controller.service();
    TEST_ASSERT_TRUE(actors.actor.output==OnOffState::Off);
    TEST_ASSERT_TRUE(controller.running());
    for(unsigned i=0;i<25;++i)frame(29000);
    frame(28000); TEST_ASSERT_TRUE(actors.actor.output==OnOffState::On);
    TEST_ASSERT_EQUAL_UINT(3,controller.wettingCount());
    actors.actor.output=OnOffState::Off; controller.service();
    TEST_ASSERT_FALSE(controller.running());
    frame(26000); TEST_ASSERT_TRUE(actors.actor.output==OnOffState::Off);
    TEST_ASSERT_GREATER_THAN_UINT(3,source.gateRestarts);
    TEST_ASSERT_TRUE(controller.begin()==ControllerOperationResult::Completed);
    for(unsigned i=0;i<25;++i)frame(34000);
    frame(30000);
    const uint32_t lastWettingMs = clock.now;
    for(unsigned i=0;i<265;++i)frame(30000);
    TEST_ASSERT_TRUE(controller.result().rain==RainDetectorRainState::EndEstimated);
    TEST_ASSERT_EQUAL_UINT32(clock.now-lastWettingMs,controller.lastWettingAgeMs());
    const uint32_t confirmedAge = controller.rainEndConfirmationAgeMs();
    frame(30000);
    TEST_ASSERT_EQUAL_UINT32(confirmedAge+250,controller.rainEndConfirmationAgeMs());
    TEST_ASSERT_TRUE(actors.actor.output==OnOffState::On);
    for(unsigned i=0;i<44;++i)frame(33000);
    TEST_ASSERT_TRUE(controller.result().dry);
    TEST_ASSERT_TRUE(actors.actor.output==OnOffState::Off);
    for(unsigned i=0;i<25;++i)frame(33000);
    frame(30000);
    TEST_ASSERT_TRUE(actors.actor.output==OnOffState::On);
    actors.actor.percent=30; // Still logically On, but no longer full heat.
    controller.service();
    TEST_ASSERT_FALSE(controller.running());
    TEST_ASSERT_TRUE(actors.actor.output==OnOffState::Off);


}
class EmptyRainMeasurements : public IMeasurementResolver {
public:
    bool latest(const MeasurementSourceReference&,MeasurementSnapshot&) const override { return false; }
};

class EventTestTime : public ITimeService {
public:
    FrequencyTestClock& clock;
    bool synced=false; time_t offset=1800000000;
    explicit EventTestTime(FrequencyTestClock& c):clock(c) {}
    void begin() override {} void loop() override {}
    bool synchronized() const override { return synced; }
    time_t now() const override { return offset+clock.now/1000; }
    bool localCivilTime(tm&) const override { return false; }
    String iso8601Utc() const override { return ""; }
    String iso8601Local() const override { return ""; }
    String iso8601Local(time_t) const override { return ""; }
    uint32_t epoch() const override { return now(); }
};
void test_rain_event_times_resolve_after_sync_and_track_each_rain_episode() {
    RainTestSource source; RainTestActors actors; FrequencyTestClock clock; RainTestLogger logger;
    EmptyRainMeasurements measurements; EventTestTime time(clock);
    ControllerFactory factory(measurements,actors,clock,logger,nullptr,&source);
    ControllerRuntime runtime(factory,logger,&time);
    ControllerSlotConfiguration slots[MaxControllerSlotCount];
    for(unsigned i=0;i<MaxControllerSlotCount;++i) { slots[i].slotId=i+1; slots[i].name="slot"; }
    slots[0].enabled=true; slots[0].implementation=ControllerImplementation::RainDetector;
    auto& config=slots[0].implementationConfiguration.rainDetector;
    config.sourceSensorId=1; config.targetActuatorId=1; config.policy.endSeconds=30;
    TEST_ASSERT_TRUE(runtime.initialize(slots));
    ControllerRuntimeInfo info;
    const auto tick=[&](float hz) {
        clock.now+=250; ++source.frame.revision; source.frame.acceptedMs=clock.now;
        source.frame.window=rainFrequencyWindow((clock.now-250)*1000ULL,clock.now*1000ULL,
            static_cast<uint32_t>(hz/4),false);
        runtime.loop(); TEST_ASSERT_TRUE(runtime.runtimeInfo(0,info));
    };
    for(unsigned i=0;i<70;++i) tick(34000);
    TEST_ASSERT_TRUE(info.rainDetector.dry);
    TEST_ASSERT_FALSE(info.rainStart.occurred); TEST_ASSERT_FALSE(info.rainEnd.occurred);
    tick(30000);
    const uint32_t start=clock.now;
    TEST_ASSERT_TRUE(info.rainStart.occurred); TEST_ASSERT_TRUE(info.rainEnd.occurred);
    TEST_ASSERT_FALSE(info.rainStart.hasTimestamp); TEST_ASSERT_FALSE(info.rainEndDetected.occurred);
    for(unsigned i=0;i<25;++i) tick(30000);
    time.synced=true; runtime.loop(); TEST_ASSERT_TRUE(runtime.runtimeInfo(0,info));
    const time_t initialStart=info.rainStart.timestamp;
    TEST_ASSERT_TRUE(info.rainStart.hasTimestamp);
    TEST_ASSERT_INT_WITHIN(1,time.offset+start/1000,initialStart);
    tick(29000);
    TEST_ASSERT_EQUAL(initialStart,info.rainStart.timestamp);
    TEST_ASSERT_TRUE(info.rainEnd.monotonicMs>info.rainStart.monotonicMs);
    const time_t lastWetting=info.rainEnd.timestamp;
    for(unsigned i=0;i<121;++i) tick(29000);
    TEST_ASSERT_TRUE(info.rainEndDetected.hasTimestamp);
    TEST_ASSERT_EQUAL(lastWetting+30,info.rainEndDetected.timestamp);
    const time_t detection=info.rainEndDetected.timestamp;
    // Refresh and subsequent clock correction must not move stored event times.
    time.offset+=5; for(unsigned i=0;i<5;++i) tick(29000);
    TEST_ASSERT_EQUAL(initialStart,info.rainStart.timestamp);
    TEST_ASSERT_EQUAL(lastWetting,info.rainEnd.timestamp);
    TEST_ASSERT_EQUAL(detection,info.rainEndDetected.timestamp);
    tick(28000);
    TEST_ASSERT_TRUE(info.rainStart.timestamp>initialStart);
    TEST_ASSERT_EQUAL(info.rainStart.timestamp,info.rainEnd.timestamp);
    TEST_ASSERT_EQUAL(detection,info.rainEndDetected.timestamp);
    runtime.stopController(1);
    runtime.startController(1); TEST_ASSERT_TRUE(runtime.runtimeInfo(0,info));
    TEST_ASSERT_FALSE(info.rainStart.occurred); TEST_ASSERT_FALSE(info.rainEndDetected.occurred);
}
void test_runtime_autostarts_enabled_rain_controller_with_heater_off() {
    RainTestSource source; RainTestActors actors; FrequencyTestClock clock; RainTestLogger logger;
    EmptyRainMeasurements measurements;
    ControllerFactory factory(measurements,actors,clock,logger,nullptr,&source);
    ControllerRuntime runtime(factory,logger);
    ControllerSlotConfiguration slots[MaxControllerSlotCount];
    for(unsigned i=0;i<MaxControllerSlotCount;++i) { slots[i].slotId=i+1; slots[i].name="slot"; }
    slots[0].enabled=true; slots[0].implementation=ControllerImplementation::RainDetector;
    slots[0].implementationConfiguration.rainDetector.sourceSensorId=1;
    slots[0].implementationConfiguration.rainDetector.targetActuatorId=1;
    TEST_ASSERT_TRUE(runtime.initialize(slots));
    ControllerRuntimeInfo info;
    TEST_ASSERT_TRUE(runtime.runtimeInfo(0,info)); TEST_ASSERT_TRUE(info.running);
    TEST_ASSERT_TRUE(actors.actor.output==OnOffState::Off);
    TEST_ASSERT_TRUE(runtime.stopController(1)==ControllerOperationResult::NoAction);
    TEST_ASSERT_TRUE(runtime.runtimeInfo(0,info)); TEST_ASSERT_FALSE(info.running);
    TEST_ASSERT_TRUE(runtime.startController(1)==ControllerOperationResult::Completed);
    TEST_ASSERT_TRUE(runtime.runtimeInfo(0,info)); TEST_ASSERT_TRUE(info.running);
    TEST_ASSERT_TRUE(runtime.rebuild(slots));
    TEST_ASSERT_TRUE(runtime.runtimeInfo(0,info)); TEST_ASSERT_TRUE(info.running);
    TEST_ASSERT_TRUE(actors.actor.output==OnOffState::Off);
}
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_startup_above_threshold_confirms_dry_without_heat_or_rain);
    RUN_TEST(test_rain_event_times_resolve_after_sync_and_track_each_rain_episode);
    RUN_TEST(test_controller_initial_drying_has_no_wetting_and_target_recovery_stays_off);
    RUN_TEST(test_startup_below_threshold_drives_one_drying_cycle_without_rain);
    RUN_TEST(test_startup_waits_for_valid_signal_and_fault_does_not_restart_heat);
    RUN_TEST(test_startup_high_then_low_restarts_confirmation_with_full_heat);
    RUN_TEST(test_shared_policy_wetting_end_and_dry_are_independent);
    RUN_TEST(test_signal_loss_aborts_cycle_but_preserves_arming);
    RUN_TEST(test_config_is_atomic_and_bounded);
    RUN_TEST(test_frequency_conversion_detects_zero_wrap_and_gate_errors);
    RUN_TEST(test_acquisition_delivers_every_window_and_fault_without_publication_poll);
    RUN_TEST(test_sensor_keeps_local_frequency_independent_of_sample_publication);
    RUN_TEST(test_frequency_metadata_does_not_reuse_ratio_measurements);
    RUN_TEST(test_controller_operates_without_UTC_and_recovers_after_signal_failure);
    RUN_TEST(test_runtime_autostarts_enabled_rain_controller_with_heater_off);
    return UNITY_END();
}
