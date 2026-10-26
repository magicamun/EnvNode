// EnvNode: see repository LICENSE.
#pragma once
#include "ILocaleConfiguration.h"
#include "ITimeConfiguration.h"
namespace EnvNode {
class IBaseSettings : public ITimeConfiguration, public ILocaleConfiguration {
public:
    virtual const DeviceConfiguration& getDeviceConfiguration() const = 0;
    virtual bool setDeviceName(const String& name) = 0;
    virtual bool setLocale(Locale locale) = 0;
    virtual bool setTimezone(const String& timezone) = 0;
    virtual bool setNtpServer1(const String& server) = 0;
    virtual bool setNtpServer2(const String& server) = 0;
};
}
