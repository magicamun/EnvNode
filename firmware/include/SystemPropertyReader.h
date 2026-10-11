#pragma once
#include "TimePropertyReader.h"
#include "IConfigurationService.h"
#include "IWiFiService.h"
#include <cstring>
namespace EnvNode {
// Build strings are supplied by the composition root; no generated build header
// is required by the property model or native tests.
class SystemPropertyReader : public IPropertyReader {
public:
    SystemPropertyReader(const ITimeService& time, const LocaleFormatter& locale,
        const char* version, const char* build, const char* commit, const char* identity,
        const IConfigurationService* configuration = nullptr, const IWiFiService* wifi = nullptr)
        : time_(time, locale), version_(version), build_(build), commit_(commit), identity_(identity), configuration_(configuration), wifi_(wifi) {}
    bool describe(const PropertyReference& reference, PropertyDescription& result) const override {
        result = PropertyDescription{};
        if (reference.componentKind != PropertyComponentKind::System || reference.componentId != 1
            || reference.propertyKey == nullptr) return false;
        const char* name = nullptr;
        if (strcmp(reference.propertyKey, "version") == 0) name = "Firmware version";
        else if (strcmp(reference.propertyKey, "build") == 0) name = "Build number";
        else if (strcmp(reference.propertyKey, "git_commit") == 0) name = "Git commit";
        else if (strcmp(reference.propertyKey, "build_identity") == 0) name = "Full build identity";
        else if (configuration_ && strcmp(reference.propertyKey, "device_name") == 0) name = "Device name";
        else if (wifi_ && strcmp(reference.propertyKey, "hostname") == 0) name = "Hostname";
        else if (wifi_ && strcmp(reference.propertyKey, "ip_address") == 0) name = "IP address";
        else return time_.describe(reference, result);
        result.stableKey = reference.propertyKey;
        result.displayName = name;
        result.valueKind = PropertyValueKind::Text;
        return true;
    }
    PropertyReadResult read(const PropertyReference& reference, PropertySnapshot& result) const override {
        result = PropertySnapshot{};
        PropertyDescription description;
        if (!describe(reference, description)) return PropertyReadResult::UnknownReference;
        String runtimeText;
        if (strcmp(reference.propertyKey, "device_name") == 0) runtimeText = configuration_->getConfiguration().device.name;
        else if (strcmp(reference.propertyKey, "hostname") == 0) runtimeText = wifi_->hostname();
        else if (strcmp(reference.propertyKey, "ip_address") == 0) {
            runtimeText = wifi_->ipAddress();
            if (runtimeText.isEmpty() || runtimeText == "0.0.0.0") return PropertyReadResult::NoValue;
        }
        else return readBuildOrTime(reference, result);
        result.value = PropertyValue::text(runtimeText);
        result.valid = true;
        return PropertyReadResult::Available;
    }
private:
    PropertyReadResult readBuildOrTime(const PropertyReference& reference, PropertySnapshot& result) const {
        const char* text = nullptr;
        if (strcmp(reference.propertyKey, "version") == 0) text = version_;
        else if (strcmp(reference.propertyKey, "build") == 0) text = build_;
        else if (strcmp(reference.propertyKey, "git_commit") == 0) text = commit_;
        else if (strcmp(reference.propertyKey, "build_identity") == 0) text = identity_;
        else return time_.read(reference, result);
        result.value = PropertyValue::text(String(text));
        result.valid = true;
        return PropertyReadResult::Available;
    }
    TimePropertyReader time_;
    const char* version_;
    const char* build_;
    const char* commit_;
    const char* identity_;
    const IConfigurationService* configuration_;
    const IWiFiService* wifi_;
};
}
