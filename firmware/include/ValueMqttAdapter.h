#pragma once
#include "ValueRuntime.h"
#include "IMqttService.h"
#include "Logger.h"
namespace EnvNode {
class ValueMqttAdapter : public IMqttMessageHandler {
public:
    ValueMqttAdapter(ILogger& logger, const IConfigurationService& configuration, IMqttService& mqtt, ValueRuntime& values)
        : logger_(logger), configuration_(configuration), mqtt_(mqtt), values_(values) {}
    void loop();
    void handleMqttMessage(const char* topic, const uint8_t* payload, size_t length) override;
private:
    ILogger& logger_;
    const IConfigurationService& configuration_;
    IMqttService& mqtt_;
    ValueRuntime& values_;
    String subscription_;
    bool failureReported_ = false;
};
}
