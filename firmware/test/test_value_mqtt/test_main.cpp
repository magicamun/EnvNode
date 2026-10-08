#include <unity.h>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "ConfigurationService.h"
#include "ValueMqttAdapter.h"
#include "ValueStatePublisher.h"
#include "ValueMqttTopic.h"
#include "MqttMessageRouter.h"
using namespace EnvNode;
HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t n) { return n; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}
void setUp() { Preferences().clear(); Preferences::failStringWrites() = false; }
void tearDown() { Preferences::failStringWrites() = false; }
class Log : public ILogger {
public:
    void begin(unsigned long) override {}
    void println(const char*) override {}
    void printf(const char*, ...) override {}
};
class Mqtt : public IMqttService {
public:
    bool online = true, allowSubscribe = true;
    unsigned int subscriptions = 0, publications = 0;
    std::string lastSubscription;
    std::map<std::string,std::string> retained;
    std::set<std::string> failures;
    IMqttMessageHandler* handler = nullptr;
    void begin() override {}
    void loop() override {}
    bool connected() const override { return online; }
    bool subscribe(const char* topic) override { ++subscriptions; lastSubscription=topic; return allowSubscribe; }
    bool publish(const char* topic, const char* payload, bool retain) override {
        ++publications; TEST_ASSERT_TRUE(retain);
        if (failures.count(topic)) return false;
        if (*payload) retained[topic]=payload; else retained.erase(topic);
        return true;
    }
    void setMessageHandler(IMqttMessageHandler* h) override { handler=h; }
};
EnumValueConfiguration mode() {
    EnumValueConfiguration c; c.id=1; c.name="Water <source>"; c.defaultCode="auto";
    c.options={{"auto","Automatik"},{"cistern","Zisterne"},{"mains","Hauswasser"}};
    return c;
}
struct Fixture {
    Log log; ConfigurationService cfg; Mqtt mqtt; ValueRuntime runtime{cfg};
    ValueMqttAdapter adapter{log,cfg,mqtt,runtime}; ValueStatePublisher publisher{log,cfg,mqtt,runtime};
    Fixture() { cfg.loadConfiguration(); runtime.begin(); runtime.saveDefinition(mode(),true); }
    String command() { return mqttValueCommandTopic(cfg.getConfiguration().device.name,1); }
    String status() { return mqttValueStatusTopic(cfg.getConfiguration().device.name,1); }
    String description() { return mqttValueDescriptionTopic(cfg.getConfiguration().device.name,1); }
    void send(const char* code) { adapter.handleMqttMessage(command().c_str(),reinterpret_cast<const uint8_t*>(code),strlen(code)); }
};
void test_command_validation_shared_runtime_and_persistence() {
    Fixture f; f.send("cistern");
    TEST_ASSERT_EQUAL_STRING("cistern",f.runtime.find(1)->current()->code.c_str());
    const char* invalid[]={"","CISTERN","cistern ","unknown","\"auto\"","auto\n","aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"};
    for(const auto* code:invalid) f.send(code);
    const uint8_t nul[]={'a','u','t','o',0,'x'};
    f.adapter.handleMqttMessage(f.command().c_str(),nul,sizeof(nul));
    TEST_ASSERT_EQUAL_STRING("cistern",f.runtime.find(1)->current()->code.c_str());
    f.adapter.handleMqttMessage("envnode/Other/value/1/cmd/state",reinterpret_cast<const uint8_t*>("auto"),4);
    TEST_ASSERT_EQUAL_STRING("cistern",f.runtime.find(1)->current()->code.c_str());
    auto c=mode();c.restartPolicy=ValueRestartPolicy::RestoreLastValue;
    TEST_ASSERT_TRUE(f.runtime.saveDefinition(c,false));f.send("mains");
    Preferences::failStringWrites()=true;f.send("cistern");
    TEST_ASSERT_EQUAL_STRING("mains",f.runtime.find(1)->current()->code.c_str());
    Preferences::failStringWrites()=false;
    ConfigurationService reboot;reboot.loadConfiguration();ValueRuntime restored(reboot);restored.begin();
    TEST_ASSERT_EQUAL_STRING("mains",restored.find(1)->current()->code.c_str());
}
void test_topic_parser_is_exact_and_bounded() {
    ValueId id=0;TEST_ASSERT_TRUE(parseMqttValueCommandTopic("envnode/Water_Node/value/65535/cmd/state","Water Node",id));
    TEST_ASSERT_EQUAL_UINT32(65535,id);
    const char* invalid[]={"envnode/Water_Node/value/0/cmd/state","envnode/Water_Node/value/01/cmd/state","envnode/Water_Node/value/65536/cmd/state","envnode/Water_Node/value/9999999999999999999/cmd/state","envnode/Water_Node/value/1/cmd/state/extra","envnode/Water_Node/value/1/status/state","envnode/Other/value/1/cmd/state"};
    for(const auto* topic:invalid) { TEST_ASSERT_FALSE(parseMqttValueCommandTopic(topic,"Water Node",id));TEST_ASSERT_EQUAL_UINT32(0,id); }
}
void test_subscription_retry_reconnect_and_device_rename() {
    Fixture f;f.mqtt.allowSubscribe=false;f.adapter.loop();f.adapter.loop();TEST_ASSERT_EQUAL_UINT32(2,f.mqtt.subscriptions);
    f.mqtt.allowSubscribe=true;f.adapter.loop();f.adapter.loop();TEST_ASSERT_EQUAL_UINT32(3,f.mqtt.subscriptions);
    f.mqtt.online=false;f.adapter.loop();f.mqtt.online=true;f.adapter.loop();TEST_ASSERT_EQUAL_UINT32(4,f.mqtt.subscriptions);
    f.cfg.setDeviceName("Renamed");f.adapter.loop();TEST_ASSERT_EQUAL_STRING("envnode/Renamed/value/+/cmd/state",f.mqtt.lastSubscription.c_str());
}
void test_retained_state_web_changes_metadata_and_reconnect() {
    Fixture f;f.publisher.loop();TEST_ASSERT_EQUAL_UINT32(2,f.mqtt.publications);
    TEST_ASSERT_EQUAL_STRING("auto",f.mqtt.retained[f.status().c_str()].c_str());
    TEST_ASSERT_NOT_NULL(strstr(f.mqtt.retained[f.description().c_str()].c_str(),"\"code\":\"cistern\""));
    f.publisher.loop();TEST_ASSERT_EQUAL_UINT32(2,f.mqtt.publications);
    f.runtime.set(1,"mains");f.publisher.loop();TEST_ASSERT_EQUAL_UINT32(3,f.mqtt.publications);
    TEST_ASSERT_EQUAL_STRING("mains",f.mqtt.retained[f.status().c_str()].c_str());
    auto c=mode();c.options[0].label="Automatic & \"default\"";f.runtime.saveDefinition(c,false);f.publisher.loop();
    const auto& description=f.mqtt.retained[f.description().c_str()];
    TEST_ASSERT_NOT_NULL(strstr(description.c_str(),"Automatic & \\\"default\\\""));
    const unsigned int previous=f.mqtt.publications;
    f.mqtt.online=false;f.publisher.loop();f.mqtt.online=true;f.publisher.loop();
    TEST_ASSERT_EQUAL_UINT32(previous+2,f.mqtt.publications);
}
void test_publish_failures_retry_and_deleted_values_are_cleared() {
    Fixture f;f.mqtt.failures.insert(f.status().c_str());f.publisher.loop();
    TEST_ASSERT_TRUE(f.mqtt.retained.count(f.status().c_str())==0);
    f.mqtt.failures.clear();f.publisher.loop();TEST_ASSERT_EQUAL_STRING("auto",f.mqtt.retained[f.status().c_str()].c_str());
    const String status=f.status(),description=f.description();
    f.runtime.remove(1);f.mqtt.failures.insert(description.c_str());f.publisher.loop();
    TEST_ASSERT_TRUE(f.mqtt.retained.count(status.c_str())==0);
    // Recreating after a partially successful tombstone must republish both topics.
    f.runtime.saveDefinition(mode(),true);f.mqtt.failures.clear();f.publisher.loop();
    TEST_ASSERT_EQUAL_STRING("auto",f.mqtt.retained[status.c_str()].c_str());
    f.mqtt.online=false;f.publisher.loop();f.runtime.remove(1);f.mqtt.online=true;f.publisher.loop();
    TEST_ASSERT_TRUE(f.mqtt.retained.empty());
}
void test_rename_clears_old_topics_and_publishes_new_topics() {
    Fixture f;f.publisher.loop();const String old=f.status();f.cfg.setDeviceName("New Name");f.publisher.loop();
    TEST_ASSERT_TRUE(f.mqtt.retained.count(old.c_str())==0);
    TEST_ASSERT_EQUAL_STRING("auto",f.mqtt.retained["envnode/New_Name/value/1/status/state"].c_str());
}
class Observer : public IMqttMessageHandler {
public: unsigned int count=0;
    void handleMqttMessage(const char*,const uint8_t*,size_t) override {++count;}
};
void test_router_delivers_to_values_alongside_existing_handlers() {
    Fixture f;Observer first,second;MqttMessageRouter router(f.mqtt,first,second,&f.adapter);router.begin();
    f.mqtt.handler->handleMqttMessage(f.command().c_str(),reinterpret_cast<const uint8_t*>("cistern"),7);
    TEST_ASSERT_EQUAL_UINT32(1,first.count);TEST_ASSERT_EQUAL_UINT32(1,second.count);
    TEST_ASSERT_EQUAL_STRING("cistern",f.runtime.find(1)->current()->code.c_str());
}
int main(int,char**) { UNITY_BEGIN();
    RUN_TEST(test_command_validation_shared_runtime_and_persistence);
    RUN_TEST(test_topic_parser_is_exact_and_bounded);
    RUN_TEST(test_subscription_retry_reconnect_and_device_rename);
    RUN_TEST(test_retained_state_web_changes_metadata_and_reconnect);
    RUN_TEST(test_publish_failures_retry_and_deleted_values_are_cleared);
    RUN_TEST(test_rename_clears_old_topics_and_publishes_new_topics);
    RUN_TEST(test_router_delivers_to_values_alongside_existing_handlers);
    return UNITY_END(); }
