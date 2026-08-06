#pragma once

namespace WeatherStation {

class IWebService {
public:
    virtual ~IWebService() = default;

    virtual void begin() = 0;
    virtual void loop() = 0;
};

} // namespace WeatherStation
