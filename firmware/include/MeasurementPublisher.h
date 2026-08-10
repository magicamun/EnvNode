#pragma once

#include <Arduino.h>

#include "IConfigurationService.h"
#include "IMeasurementSink.h"
#include "IMqttService.h"
#include "ITimeService.h"
#include "PresentationUnit.h"

namespace EnvNode {

class MeasurementPublisher : public IMeasurementSink {
public:
    MeasurementPublisher(
        IConfigurationService& configurationService,
        ITimeService& timeService,
        IMqttService& mqttService);

    void emit(const Measurement& measurement) override;

private:
    static const char* qualityName(MeasurementQuality quality);
    static String serializePayload(
        const Measurement& measurement,
        const String& timestamp,
        PresentationUnit presentationUnit,
        float presentationValue,
        bool hasPresentationValue);

    IConfigurationService& configurationService_;
    ITimeService& timeService_;
    IMqttService& mqttService_;
};

} // namespace EnvNode
