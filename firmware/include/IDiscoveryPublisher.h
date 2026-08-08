#pragma once

namespace WeatherStation {

class IDiscoveryPublisher {
public:
    virtual ~IDiscoveryPublisher() = default;
    virtual void loop() = 0;
};

} // namespace WeatherStation
