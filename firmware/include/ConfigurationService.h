#pragma once

#include <Preferences.h>
#include "Configuration.h"
#include "IConfigurationService.h"

namespace EnvNode {

class ConfigurationService : public IConfigurationService {
public:
    void loadConfiguration() override;
    const Configuration& getConfiguration() const override;
    Locale getLocale() const override;

    bool setDeviceName(const String& deviceName) override;
    bool setNetworkConfiguration(const NetworkConfiguration& network, bool updatePassword) override;
    bool setWifiSSID(const String& ssid) override;
    bool setWifiPassword(const String& password) override;
    bool setMqttServer(const String& server) override;
    bool setMqttPort(uint16_t port) override;
    bool setMqttUsername(const String& username) override;
    bool setMqttPassword(const String& password) override;
    bool setTimezone(const String& timezone) override;
    bool setNtpServer1(const String& server) override;
    bool setNtpServer2(const String& server) override;
    bool setLocale(Locale locale) override;
    bool setPresentationUnit(MeasurementType type, PresentationUnit unit) override;
    bool setSensorSlotConfiguration(const SensorSlotConfiguration& slot) override;
    bool setActuatorSlotConfiguration(const ActuatorSlotConfiguration& slot) override;
    bool setControllerSlotConfiguration(const ControllerSlotConfiguration& slot) override;
    bool setDisplayConfiguration(const DisplayConfiguration& display) override;
    bool setEnumValueDefinitions(const std::vector<EnumValueConfiguration>& definitions) override;
    bool saveEnumValueCode(ValueId id, const String& code) override;
    bool resetToDefaults() override;

private:
    bool persistValues(const ValueConfiguration& values);
    void loadValues();
    void initializeDefaults();
    void loadFromPreferences();
    void validateConfiguration();
    void ensurePreferencesStarted();
    bool persistString(const char* key, const String& value);
    bool persistUInt(const char* key, uint32_t value);
    bool persistFloat(const char* key, float value);
    PresentationUnit loadPresentationUnit(const char* key, MeasurementType type);
    bool validateDeviceName(const String& deviceName) const;
    bool validateWifiSSID(const String& ssid) const;
    bool validateWifiPassword(const String& password) const;
    bool validateMqttServer(const String& server) const;
    bool validateMqttPort(uint16_t port) const;
    bool validateMqttUsername(const String& username) const;
    bool validateMqttPassword(const String& password) const;
    bool validateTimezone(const String& timezone) const;
    bool validateNtpServer(const String& server) const;
    bool validateHostname(const String& hostname) const;
    bool validateNetworkConfiguration(const NetworkConfiguration& network) const;
    bool validateIPv4(const String& value, bool allowEmpty = false) const;
    void initializeSensorDefaults();
    void loadSensorSlots();
    bool persistSensorSlot(const SensorSlotConfiguration& slot);
    bool validateSensorSlot(const SensorSlotConfiguration& slot) const;
    bool validateSensorSlots(const SensorSlotConfiguration* slots) const;
    void initializeActuatorDefaults();
    void loadActuatorSlots();
    bool persistActuatorSlot(const ActuatorSlotConfiguration& slot);
    bool validateActuatorSlot(const ActuatorSlotConfiguration& slot) const;
    bool validateActuatorSlots(const ActuatorSlotConfiguration* slots) const;
    void initializeControllerDefaults();
    void loadControllerSlots();
    bool persistControllerSlot(const ControllerSlotConfiguration& slot);
    bool validateControllerSlot(
        const ControllerSlotConfiguration& slot,
        const SensorSlotConfiguration* sensorSlots,
        const ActuatorSlotConfiguration* actuatorSlots) const;
    bool validateControllerSlots(
        const ControllerSlotConfiguration* controllerSlots,
        const SensorSlotConfiguration* sensorSlots,
        const ActuatorSlotConfiguration* actuatorSlots) const;
    bool validateHardwareOccupancy(
        const SensorSlotConfiguration* sensorSlots,
        const ActuatorSlotConfiguration* actuatorSlots,
        const DisplayConfiguration* display = nullptr) const;

    Configuration configuration_;
    Preferences preferences_;
    bool preferencesInitialized_ = false;
};

} // namespace EnvNode
