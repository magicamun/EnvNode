#pragma once
#include "ValueRuntime.h"
#include "IMqttService.h"
#include "Logger.h"
namespace EnvNode {
String buildValueMqttDescription(const String& deviceName, const EnumValueConfiguration& definition);
class ValueStatePublisher {
public:
    ValueStatePublisher(ILogger& logger, const IConfigurationService& configuration, IMqttService& mqtt, const ValueRuntime& values)
        : logger_(logger), configuration_(configuration), mqtt_(mqtt), values_(values) {}
    void loop();
private:
    struct Published {
        ValueId id = 0;
        String deviceName;
        String state;
        uint32_t descriptionRevision = 0;
        bool stateKnown = false;
        bool descriptionKnown = false;
        bool stateCleared = false;
        bool descriptionCleared = false;
    };
    ILogger& logger_;
    const IConfigurationService& configuration_;
    IMqttService& mqtt_;
    const ValueRuntime& values_;
    std::vector<Published> published_;
    bool wasConnected_ = false;
    bool failureReported_ = false;
};
}
