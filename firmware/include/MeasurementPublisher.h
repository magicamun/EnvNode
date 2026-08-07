#pragma once

#include <Arduino.h>

#include "IConfigurationService.h"
#include "IMeasurementSink.h"
#include "IMqttService.h"
#include "ITimeService.h"

namespace WeatherStation {

class MeasurementPublisher : public IMeasurementSink {
public:
    MeasurementPublisher(
        IConfigurationService& configurationService,
        ITimeService& timeService,
        IMqttService& mqttService);

    void emit(const Measurement& measurement) override;

private:
    static const char* measurementTypeTopic(MeasurementType type);
    static const char* qualityName(MeasurementQuality quality);
    static String topicSafeDeviceName(const String& deviceName);
    static String serializePayload(const Measurement& measurement, const String& timestamp);

    IConfigurationService& configurationService_;
    ITimeService& timeService_;
    IMqttService& mqttService_;
};

} // namespace WeatherStation
