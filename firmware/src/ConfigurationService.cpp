#include "ConfigurationService.h"
#include <Arduino.h>
#include <Preferences.h>
#include <IPAddress.h>
#include "SensorImplementationRegistry.h"
#include "ActuatorImplementationRegistry.h"
#include "ControllerImplementationRegistry.h"
#include "HardwareResources.h"
#include <cmath>

namespace EnvNode {

namespace {
constexpr const char* PreferencesNamespace = "weather";
constexpr const char* KeyDeviceName = "deviceName";
constexpr const char* KeyWifiSSID = "wifiSSID";
constexpr const char* KeyWifiPassword = "wifiPassword";
constexpr const char* KeyHostname = "hostname";
constexpr const char* KeyAddressMode = "addressMode";
constexpr const char* KeyIpv4Address = "ipv4Address";
constexpr const char* KeySubnetMask = "subnetMask";
constexpr const char* KeyGateway = "gateway";
constexpr const char* KeyDns1 = "dns1";
constexpr const char* KeyDns2 = "dns2";
constexpr const char* KeyMqttServer = "mqttServer";
constexpr const char* KeyMqttPort = "mqttPort";
constexpr const char* KeyMqttUsername = "mqttUsername";
constexpr const char* KeyMqttPassword = "mqttPassword";
constexpr const char* KeyTimezone = "timezone";
constexpr const char* KeyNtpServer1 = "ntpServer1";
constexpr const char* KeyNtpServer2 = "ntpServer2";
constexpr const char* KeyLocale = "locale";
constexpr const char* KeyTemperatureUnit = "unitTemp";
constexpr const char* KeyPressureUnit = "unitPressure";
constexpr const char* KeySolarCellTemperatureUnit = "unitSolarTemp";
constexpr const char* KeyRainDetectorLevelUnit = "unitRainLevel";
constexpr uint16_t DefaultMqttPort = 1883;
constexpr const char* DefaultTimezone = "CET-1CEST,M3.5.0/2,M10.5.0/3";
constexpr const char* DefaultNtpServer1 = "pool.ntp.org";
constexpr const char* DefaultNtpServer2 = "time.cloudflare.com";
constexpr size_t MaxDeviceNameLength = 32;
constexpr size_t MaxWifiSSIDLength = 32;
constexpr size_t MaxWifiPasswordLength = 64;
constexpr size_t MaxMqttServerLength = 64;
constexpr size_t MaxMqttUsernameLength = 32;
constexpr size_t MaxMqttPasswordLength = 64;
constexpr size_t MaxTimezoneLength = 128;
constexpr size_t MaxNtpServerLength = 64;
constexpr size_t MaxHostnameLength = 63;

String sensorKey(SensorId slotId, const char* field) {
    return "s" + String(slotId) + "_" + field;
}

String actuatorKey(ActuatorId slotId, const char* field) {
    return "a" + String(slotId) + "_" + field;
}

String controllerKey(ControllerId slotId, const char* field) {
    return "c" + String(slotId) + "_" + field;
}
}

void ConfigurationService::ensurePreferencesStarted() {
    if (!preferencesInitialized_) {
        preferences_.begin(PreferencesNamespace, false);
        preferencesInitialized_ = true;
    }
}

void ConfigurationService::initializeDefaults() {
    configuration_.device.name = "WeatherStation";
    configuration_.network.hostname = "WeatherStation";
    configuration_.network.wifiSSID = String();
    configuration_.network.wifiPassword = String();
    configuration_.network.addressMode = NetworkAddressMode::Dhcp;
    configuration_.network.ipv4Address = String();
    configuration_.network.subnetMask = String();
    configuration_.network.gateway = String();
    configuration_.network.dns1 = String();
    configuration_.network.dns2 = String();
    configuration_.mqtt.server = String();
    configuration_.mqtt.port = DefaultMqttPort;
    configuration_.mqtt.username = String();
    configuration_.mqtt.password = String();
    configuration_.time.timezone = String(DefaultTimezone);
    configuration_.time.ntpServer1 = String(DefaultNtpServer1);
    configuration_.time.ntpServer2 = String(DefaultNtpServer2);
    configuration_.locale.locale = Locale::GermanGermany;
    configuration_.presentation.temperature =
        measurementTypeMetadata(MeasurementType::Temperature).defaultPresentationUnit;
    configuration_.presentation.atmosphericPressure =
        measurementTypeMetadata(MeasurementType::AtmosphericPressure).defaultPresentationUnit;
    configuration_.presentation.solarCellTemperature =
        measurementTypeMetadata(MeasurementType::SolarCellTemperature).defaultPresentationUnit;
    configuration_.presentation.rainDetectorLevel =
        measurementTypeMetadata(MeasurementType::RainDetectorLevel).defaultPresentationUnit;
    initializeSensorDefaults();
    initializeActuatorDefaults();
    initializeControllerDefaults();
}

void ConfigurationService::initializeSensorDefaults() {
    for (size_t index = 0; index < MaxSensorSlotCount; ++index) {
        SensorSlotConfiguration& slot = configuration_.sensorSlots[index];
        slot.slotId = static_cast<SensorId>(index + 1);
        slot.enabled = false;
        slot.name = "Slot " + String(index + 1);
        slot.implementation = SensorImplementation::None;
        slot.schedule = SensorSchedule::eventOnly(false);
        slot.hardware = HardwareResourceAssignment::none();
        slot.implementationConfiguration.am2302 = AM2302Configuration();
        slot.implementationConfiguration.bme280 = BME280Configuration();
        slot.implementationConfiguration.sht4x = SHT4xConfiguration();
        slot.implementationConfiguration.shtc3 = SHTC3Configuration();
        slot.implementationConfiguration.rainGauge = RainGaugeConfiguration();
    }

    SensorSlotConfiguration& temperature = configuration_.sensorSlots[0];
    temperature.enabled = true;
    temperature.name = "Simulated Temperature";
    temperature.implementation = SensorImplementation::SimulatedTemperature;
    temperature.schedule = SensorSchedule::periodic(5000);

    SensorSlotConfiguration& humidity = configuration_.sensorSlots[1];
    humidity.enabled = true;
    humidity.name = "Simulated Humidity";
    humidity.implementation = SensorImplementation::SimulatedHumidity;
    humidity.schedule = SensorSchedule::periodic(5000);

    SensorSlotConfiguration& pressure = configuration_.sensorSlots[2];
    pressure.enabled = true;
    pressure.name = "Simulated Barometer";
    pressure.implementation = SensorImplementation::SimulatedPressure;
    pressure.schedule = SensorSchedule::periodic(10000);

    SensorSlotConfiguration& am2302 = configuration_.sensorSlots[3];
    am2302.enabled = true;
    am2302.name = "Outside";
    am2302.implementation = SensorImplementation::AM2302;
    am2302.schedule = SensorSchedule::periodic(5000);
    am2302.hardware = HardwareResourceAssignment::gpioResource(GpioResource(4));
    am2302.implementationConfiguration.am2302 = AM2302Configuration(GpioResource(4));
}

void ConfigurationService::initializeActuatorDefaults() {
    for (size_t index = 0; index < MaxActuatorSlotCount; ++index) {
        ActuatorSlotConfiguration& slot = configuration_.actuatorSlots[index];
        slot.slotId = static_cast<ActuatorId>(index + 1);
        slot.enabled = false;
        slot.name = "Actuator Slot " + String(index + 1);
        slot.implementation = ActuatorImplementation::None;
        slot.hardware = HardwareResourceAssignment::none();
    }
}

void ConfigurationService::initializeControllerDefaults() {
    for (size_t index = 0; index < MaxControllerSlotCount; ++index) {
        ControllerSlotConfiguration& slot = configuration_.controllerSlots[index];
        slot.slotId = static_cast<ControllerId>(index + 1);
        slot.enabled = false;
        slot.name = "Controller Slot " + String(index + 1);
        slot.implementation = ControllerImplementation::None;
        slot.implementationConfiguration.blink = BlinkControllerConfiguration{};
        slot.implementationConfiguration.threshold = ThresholdControllerConfiguration{};
    }
}

void ConfigurationService::loadFromPreferences() {
    ensurePreferencesStarted();

    if (preferences_.isKey(KeyDeviceName)) {
        configuration_.device.name = preferences_.getString(KeyDeviceName, configuration_.device.name);
    }
    if (preferences_.isKey(KeyWifiSSID)) {
        configuration_.network.wifiSSID = preferences_.getString(KeyWifiSSID, configuration_.network.wifiSSID);
    }
    if (preferences_.isKey(KeyWifiPassword)) {
        configuration_.network.wifiPassword = preferences_.getString(KeyWifiPassword, configuration_.network.wifiPassword);
    }
    configuration_.network.hostname = preferences_.getString(KeyHostname, configuration_.device.name);
    const uint32_t addressMode = preferences_.getUInt(KeyAddressMode, 0);
    configuration_.network.addressMode = addressMode == 1
        ? NetworkAddressMode::Static : NetworkAddressMode::Dhcp;
    configuration_.network.ipv4Address = preferences_.getString(KeyIpv4Address, "");
    configuration_.network.subnetMask = preferences_.getString(KeySubnetMask, "");
    configuration_.network.gateway = preferences_.getString(KeyGateway, "");
    configuration_.network.dns1 = preferences_.getString(KeyDns1, "");
    if (preferences_.isKey(KeyDns2)) {
        configuration_.network.dns2 = preferences_.getString(KeyDns2, "");
    }
    if (preferences_.isKey(KeyMqttServer)) {
        configuration_.mqtt.server = preferences_.getString(KeyMqttServer, configuration_.mqtt.server);
    }
    if (preferences_.isKey(KeyMqttPort)) {
        configuration_.mqtt.port = static_cast<uint16_t>(preferences_.getUInt(KeyMqttPort, configuration_.mqtt.port));
    }
    if (preferences_.isKey(KeyMqttUsername)) {
        configuration_.mqtt.username = preferences_.getString(KeyMqttUsername, configuration_.mqtt.username);
    }
    if (preferences_.isKey(KeyMqttPassword)) {
        configuration_.mqtt.password = preferences_.getString(KeyMqttPassword, configuration_.mqtt.password);
    }
    if (preferences_.isKey(KeyTimezone)) {
        configuration_.time.timezone = preferences_.getString(KeyTimezone, configuration_.time.timezone);
    }
    if (preferences_.isKey(KeyNtpServer1)) {
        configuration_.time.ntpServer1 = preferences_.getString(KeyNtpServer1, configuration_.time.ntpServer1);
    }
    if (preferences_.isKey(KeyNtpServer2)) {
        configuration_.time.ntpServer2 = preferences_.getString(KeyNtpServer2, configuration_.time.ntpServer2);
    }
    const uint32_t storedLocale = preferences_.getUInt(
        KeyLocale, static_cast<uint32_t>(Locale::GermanGermany));
    configuration_.locale.locale = storedLocale <= static_cast<uint32_t>(Locale::EnglishUnitedStates)
        ? static_cast<Locale>(storedLocale)
        : Locale::GermanGermany;
    configuration_.presentation.temperature =
        loadPresentationUnit(KeyTemperatureUnit, MeasurementType::Temperature);
    configuration_.presentation.atmosphericPressure =
        loadPresentationUnit(KeyPressureUnit, MeasurementType::AtmosphericPressure);
    configuration_.presentation.solarCellTemperature =
        loadPresentationUnit(KeySolarCellTemperatureUnit, MeasurementType::SolarCellTemperature);
    configuration_.presentation.rainDetectorLevel =
        loadPresentationUnit(KeyRainDetectorLevelUnit, MeasurementType::RainDetectorLevel);
    loadSensorSlots();
    loadActuatorSlots();
    loadControllerSlots();
}

void ConfigurationService::loadSensorSlots() {
    SensorSlotConfiguration loaded[MaxSensorSlotCount];
    for (size_t index = 0; index < MaxSensorSlotCount; ++index) {
        loaded[index] = configuration_.sensorSlots[index];
        const SensorId expectedId = static_cast<SensorId>(index + 1);
        const String implementationKey = sensorKey(expectedId, "impl");
        if (!preferences_.isKey(implementationKey.c_str())) continue;

        loaded[index].slotId = static_cast<SensorId>(preferences_.getUInt(
            sensorKey(expectedId, "id").c_str(), expectedId));
        loaded[index].enabled = preferences_.getUInt(
            sensorKey(expectedId, "en").c_str(), loaded[index].enabled ? 1 : 0) != 0;
        loaded[index].name = preferences_.getString(
            sensorKey(expectedId, "name").c_str(), loaded[index].name);
        const String stableImplementation = preferences_.getString(
            implementationKey.c_str(), "none");
        const SensorImplementationMetadata* metadata =
            SensorImplementationRegistry::findByStableId(stableImplementation.c_str());
        loaded[index].implementation = metadata == nullptr
            ? static_cast<SensorImplementation>(255) : metadata->implementation;
        const uint32_t mode = preferences_.getUInt(sensorKey(expectedId, "mode").c_str(), 0);
        loaded[index].schedule.acquisitionMode = mode == 1
            ? AcquisitionMode::Periodic : AcquisitionMode::EventOnly;
        loaded[index].schedule.sampleIntervalMs = preferences_.getUInt(
            sensorKey(expectedId, "int").c_str(), 0);
        loaded[index].schedule.enabled = loaded[index].enabled;
        const uint8_t gpio = static_cast<uint8_t>(preferences_.getUInt(
            sensorKey(expectedId, "gpio").c_str(), 0));
        const uint8_t i2cAddress = static_cast<uint8_t>(preferences_.getUInt(
            sensorKey(expectedId, "i2caddr").c_str(), 0x76));
        const uint32_t storedI2CBus = preferences_.getUInt(
            sensorKey(expectedId, "i2cbus").c_str(), static_cast<uint32_t>(I2CBus::I2C0));
        const I2CBus i2cBus = storedI2CBus == static_cast<uint32_t>(I2CBus::I2C1)
            ? I2CBus::I2C1 : I2CBus::I2C0;
        if (loaded[index].implementation == SensorImplementation::AM2302
            || loaded[index].implementation == SensorImplementation::RainGauge) {
            loaded[index].hardware = HardwareResourceAssignment::gpioResource(GpioResource(gpio));
        } else if (loaded[index].implementation == SensorImplementation::BME280
            || loaded[index].implementation == SensorImplementation::SHT4x
            || loaded[index].implementation == SensorImplementation::SHTC3) {
            loaded[index].hardware = HardwareResourceAssignment::i2cResource(
                I2CResource(i2cBus, i2cAddress));
        } else {
            loaded[index].hardware = HardwareResourceAssignment::none();
        }
        loaded[index].implementationConfiguration.am2302 = AM2302Configuration(GpioResource(gpio));
        loaded[index].implementationConfiguration.bme280 = BME280Configuration(
            I2CResource(i2cBus, i2cAddress));
        loaded[index].implementationConfiguration.sht4x = SHT4xConfiguration(
            I2CResource(i2cBus, i2cAddress));
        loaded[index].implementationConfiguration.shtc3 = SHTC3Configuration(
            I2CResource(i2cBus, i2cAddress));
        loaded[index].implementationConfiguration.rainGauge = RainGaugeConfiguration(
            GpioResource(gpio),
            preferences_.getFloat(sensorKey(expectedId, "rgmm").c_str(), 0.2794F),
            preferences_.getUInt(sensorKey(expectedId, "rgdeb").c_str(), 50));
    }

    if (validateSensorSlots(loaded)
        && validateHardwareOccupancy(loaded, configuration_.actuatorSlots)) {
        for (size_t index = 0; index < MaxSensorSlotCount; ++index) {
            configuration_.sensorSlots[index] = loaded[index];
        }
    }
}

void ConfigurationService::loadActuatorSlots() {
    ActuatorSlotConfiguration loaded[MaxActuatorSlotCount];
    for (size_t index = 0; index < MaxActuatorSlotCount; ++index) {
        loaded[index] = configuration_.actuatorSlots[index];
        const ActuatorId expectedId = static_cast<ActuatorId>(index + 1);
        const String implementationKey = actuatorKey(expectedId, "impl");
        if (!preferences_.isKey(implementationKey.c_str())) continue;

        loaded[index].slotId = static_cast<ActuatorId>(preferences_.getUInt(
            actuatorKey(expectedId, "id").c_str(), expectedId));
        loaded[index].enabled = preferences_.getUInt(
            actuatorKey(expectedId, "en").c_str(), loaded[index].enabled ? 1 : 0) != 0;
        loaded[index].name = preferences_.getString(
            actuatorKey(expectedId, "name").c_str(), loaded[index].name);
        const String stableImplementation = preferences_.getString(
            implementationKey.c_str(), "none");
        const ActuatorImplementationMetadata* metadata =
            ActuatorImplementationRegistry::findByStableId(stableImplementation.c_str());
        loaded[index].implementation = metadata == nullptr
            ? static_cast<ActuatorImplementation>(255) : metadata->implementation;
        const uint8_t gpio = static_cast<uint8_t>(preferences_.getUInt(
            actuatorKey(expectedId, "gpio").c_str(), 0));
        loaded[index].hardware = metadata != nullptr
                && metadata->interfaceKind == HardwareInterfaceKind::GPIO
            ? HardwareResourceAssignment::gpioResource(GpioResource(gpio))
            : HardwareResourceAssignment::none();
    }

    if (validateActuatorSlots(loaded)
        && validateHardwareOccupancy(configuration_.sensorSlots, loaded)) {
        for (size_t index = 0; index < MaxActuatorSlotCount; ++index) {
            configuration_.actuatorSlots[index] = loaded[index];
        }
    }
}

void ConfigurationService::loadControllerSlots() {
    ControllerSlotConfiguration loaded[MaxControllerSlotCount];
    for (size_t index = 0; index < MaxControllerSlotCount; ++index) {
        loaded[index] = configuration_.controllerSlots[index];
        const ControllerId expectedId = static_cast<ControllerId>(index + 1);
        const String implementationKey = controllerKey(expectedId, "impl");
        if (!preferences_.isKey(implementationKey.c_str())) continue;

        loaded[index].slotId = static_cast<ControllerId>(preferences_.getUInt(
            controllerKey(expectedId, "id").c_str(), expectedId));
        loaded[index].enabled = preferences_.getUInt(
            controllerKey(expectedId, "en").c_str(), loaded[index].enabled ? 1 : 0) != 0;
        loaded[index].name = preferences_.getString(
            controllerKey(expectedId, "name").c_str(), loaded[index].name);
        const String stableImplementation = preferences_.getString(
            implementationKey.c_str(), "none");
        const ControllerImplementationMetadata* metadata =
            ControllerImplementationRegistry::findByStableId(stableImplementation.c_str());
        loaded[index].implementation = metadata == nullptr
            ? static_cast<ControllerImplementation>(255) : metadata->implementation;
        loaded[index].implementationConfiguration.blink.targetActuatorId =
            static_cast<ActuatorId>(preferences_.getUInt(
                controllerKey(expectedId, "act").c_str(), InvalidActuatorId));
        loaded[index].implementationConfiguration.blink.onDurationMs = preferences_.getUInt(
            controllerKey(expectedId, "onms").c_str(), 1000);
        loaded[index].implementationConfiguration.blink.offDurationMs = preferences_.getUInt(
            controllerKey(expectedId, "offms").c_str(), 1000);
        ThresholdControllerConfiguration& threshold =
            loaded[index].implementationConfiguration.threshold;
        threshold.source.sensorId = static_cast<SensorId>(preferences_.getUInt(
            controllerKey(expectedId, "src").c_str(), InvalidSensorId));
        threshold.source.measurementType = measurementTypeFromStableId(
            preferences_.getString(
                controllerKey(expectedId, "type").c_str(), "unknown").c_str());
        threshold.targetActuatorId = static_cast<ActuatorId>(preferences_.getUInt(
            controllerKey(expectedId, "tact").c_str(), InvalidActuatorId));
        threshold.onThreshold = preferences_.getFloat(
            controllerKey(expectedId, "on").c_str(), 70.0F);
        threshold.offThreshold = preferences_.getFloat(
            controllerKey(expectedId, "off").c_str(), 65.0F);
        threshold.maxMeasurementAgeMs = preferences_.getUInt(
            controllerKey(expectedId, "age").c_str(), 15000);
    }

    if (validateControllerSlots(
            loaded, configuration_.sensorSlots, configuration_.actuatorSlots)) {
        for (size_t index = 0; index < MaxControllerSlotCount; ++index) {
            configuration_.controllerSlots[index] = loaded[index];
        }
    }
}

PresentationUnit ConfigurationService::loadPresentationUnit(const char* key, MeasurementType type) {
    const PresentationUnit fallback = measurementTypeMetadata(type).defaultPresentationUnit;
    if (!preferences_.isKey(key)) {
        return fallback;
    }

    const uint32_t encoded = preferences_.getUInt(key, static_cast<uint32_t>(fallback));
    if (encoded > static_cast<uint32_t>(PresentationUnit::Ratio)) {
        return fallback;
    }
    const PresentationUnit unit = static_cast<PresentationUnit>(encoded);
    return supportsPresentationUnit(type, unit) ? unit : fallback;
}

void ConfigurationService::validateConfiguration() {
    if (!validateDeviceName(configuration_.device.name)) {
        configuration_.device.name = "WeatherStation";
    }
    if (!validateNetworkConfiguration(configuration_.network)) {
        configuration_.network.addressMode = NetworkAddressMode::Dhcp;
        if (!validateHostname(configuration_.network.hostname)) configuration_.network.hostname = "WeatherStation";
        if (!validateWifiSSID(configuration_.network.wifiSSID)) configuration_.network.wifiSSID = String();
        if (!validateWifiPassword(configuration_.network.wifiPassword)) configuration_.network.wifiPassword = String();
    }
    if (!validateMqttServer(configuration_.mqtt.server)) {
        configuration_.mqtt.server = String();
    }
    if (!validateMqttPort(configuration_.mqtt.port)) {
        configuration_.mqtt.port = DefaultMqttPort;
    }
    if (!validateMqttUsername(configuration_.mqtt.username)) {
        configuration_.mqtt.username = String();
    }
    if (!validateMqttPassword(configuration_.mqtt.password)) {
        configuration_.mqtt.password = String();
    }
    if (!validateTimezone(configuration_.time.timezone)) {
        configuration_.time.timezone = String(DefaultTimezone);
    }
    if (!validateNtpServer(configuration_.time.ntpServer1)) {
        configuration_.time.ntpServer1 = String(DefaultNtpServer1);
    }
    if (!validateNtpServer(configuration_.time.ntpServer2)) {
        configuration_.time.ntpServer2 = String(DefaultNtpServer2);
    }
}

void ConfigurationService::loadConfiguration() {
    initializeDefaults();
    loadFromPreferences();
    validateConfiguration();
}

const Configuration& ConfigurationService::getConfiguration() const {
    return configuration_;
}

Locale ConfigurationService::getLocale() const {
    return configuration_.locale.locale;
}

bool ConfigurationService::persistString(const char* key, const String& value) {
    ensurePreferencesStarted();
    if (value.isEmpty()) {
        if (!preferences_.isKey(key)) {
            return true;
        }
        return preferences_.remove(key);
    }
    return preferences_.putString(key, value) > 0;
}

bool ConfigurationService::persistUInt(const char* key, uint32_t value) {
    ensurePreferencesStarted();
    return preferences_.putUInt(key, value);
}

bool ConfigurationService::persistFloat(const char* key, float value) {
    ensurePreferencesStarted();
    return preferences_.putFloat(key, value) > 0;
}

bool ConfigurationService::validateDeviceName(const String& deviceName) const {
    return !deviceName.isEmpty() && deviceName.length() <= MaxDeviceNameLength;
}

bool ConfigurationService::validateWifiSSID(const String& ssid) const {
    return ssid.length() <= MaxWifiSSIDLength;
}

bool ConfigurationService::validateWifiPassword(const String& password) const {
    return password.length() <= MaxWifiPasswordLength;
}

bool ConfigurationService::validateMqttServer(const String& server) const {
    if (server.isEmpty()) return true;
    if (server.length() > MaxMqttServerLength) return false;
    // reject whitespace-only strings
    for (size_t i = 0; i < server.length(); ++i) {
        if (!isspace(server[i])) return true;
    }
    return false;
}

bool ConfigurationService::validateMqttPort(uint16_t port) const {
    return port >= 1 && port <= 65535;
}

bool ConfigurationService::validateMqttUsername(const String& username) const {
    return username.length() <= MaxMqttUsernameLength;
}

bool ConfigurationService::validateMqttPassword(const String& password) const {
    return password.length() <= MaxMqttPasswordLength;
}

bool ConfigurationService::validateTimezone(const String& timezone) const {
    return timezone.length() <= MaxTimezoneLength;
}

bool ConfigurationService::validateNtpServer(const String& server) const {
    return server.length() <= MaxNtpServerLength;
}

bool ConfigurationService::validateHostname(const String& hostname) const {
    if (hostname.isEmpty() || hostname.length() > MaxHostnameLength) return false;
    if (hostname[0] == '-' || hostname[hostname.length() - 1] == '-') return false;
    for (size_t index = 0; index < hostname.length(); ++index) {
        const char character = hostname[index];
        if (!isalnum(character) && character != '-') return false;
    }
    return true;
}

bool ConfigurationService::validateIPv4(const String& value, bool allowEmpty) const {
    if (allowEmpty && value.isEmpty()) return true;
    IPAddress address;
    return address.fromString(value) && static_cast<uint32_t>(address) != 0;
}

bool ConfigurationService::validateNetworkConfiguration(const NetworkConfiguration& network) const {
    if (!validateHostname(network.hostname)
        || !validateWifiSSID(network.wifiSSID)
        || !validateWifiPassword(network.wifiPassword)) {
        return false;
    }
    if (network.addressMode == NetworkAddressMode::Dhcp) return true;
    if (network.addressMode != NetworkAddressMode::Static
        || !validateIPv4(network.ipv4Address)
        || !validateIPv4(network.subnetMask)
        || !validateIPv4(network.gateway)
        || !validateIPv4(network.dns1)
        || !validateIPv4(network.dns2, true)) {
        return false;
    }

    IPAddress address, mask, gateway;
    address.fromString(network.ipv4Address);
    mask.fromString(network.subnetMask);
    gateway.fromString(network.gateway);
    bool zeroSeen = false;
    for (uint8_t byteIndex = 0; byteIndex < 4; ++byteIndex) {
        for (int bit = 7; bit >= 0; --bit) {
            const bool set = (mask[byteIndex] & (1U << bit)) != 0;
            if (!set) zeroSeen = true;
            else if (zeroSeen) return false;
        }
        if ((address[byteIndex] & mask[byteIndex]) != (gateway[byteIndex] & mask[byteIndex])) {
            return false;
        }
    }
    return true;
}

bool ConfigurationService::setDeviceName(const String& deviceName) {
    if (!validateDeviceName(deviceName)) {
        return false;
    }
    if (!persistString(KeyDeviceName, deviceName)) {
        return false;
    }
    configuration_.device.name = deviceName;
    return true;
}

bool ConfigurationService::setNetworkConfiguration(
    const NetworkConfiguration& requestedNetwork,
    bool updatePassword) {
    NetworkConfiguration network = requestedNetwork;
    if (!updatePassword) network.wifiPassword = configuration_.network.wifiPassword;
    if (!validateNetworkConfiguration(network)) return false;

    if (!persistString(KeyHostname, network.hostname)
        || !persistString(KeyWifiSSID, network.wifiSSID)
        || (updatePassword && !persistString(KeyWifiPassword, network.wifiPassword))
        || !persistUInt(KeyAddressMode, static_cast<uint32_t>(network.addressMode))
        || !persistString(KeyIpv4Address, network.ipv4Address)
        || !persistString(KeySubnetMask, network.subnetMask)
        || !persistString(KeyGateway, network.gateway)
        || !persistString(KeyDns1, network.dns1)
        || !persistString(KeyDns2, network.dns2)) {
        return false;
    }
    configuration_.network = network;
    return true;
}

bool ConfigurationService::setWifiSSID(const String& ssid) {
    if (!validateWifiSSID(ssid)) {
        return false;
    }
    if (!persistString(KeyWifiSSID, ssid)) {
        return false;
    }
    configuration_.network.wifiSSID = ssid;
    return true;
}

bool ConfigurationService::setWifiPassword(const String& password) {
    if (!validateWifiPassword(password)) {
        return false;
    }
    if (!persistString(KeyWifiPassword, password)) {
        return false;
    }
    configuration_.network.wifiPassword = password;
    return true;
}

bool ConfigurationService::setMqttServer(const String& server) {
    if (!validateMqttServer(server)) {
        return false;
    }
    if (!persistString(KeyMqttServer, server)) {
        return false;
    }
    configuration_.mqtt.server = server;
    return true;
}

bool ConfigurationService::setMqttPort(uint16_t port) {
    if (!validateMqttPort(port)) {
        return false;
    }
    if (!persistUInt(KeyMqttPort, port)) {
        return false;
    }
    configuration_.mqtt.port = port;
    return true;
}

bool ConfigurationService::setMqttUsername(const String& username) {
    if (!validateMqttUsername(username)) {
        return false;
    }
    if (!persistString(KeyMqttUsername, username)) {
        return false;
    }
    configuration_.mqtt.username = username;
    return true;
}

bool ConfigurationService::setMqttPassword(const String& password) {
    if (!validateMqttPassword(password)) {
        return false;
    }
    if (!persistString(KeyMqttPassword, password)) {
        return false;
    }
    configuration_.mqtt.password = password;
    return true;
}

bool ConfigurationService::setTimezone(const String& timezone) {
    if (!validateTimezone(timezone)) {
        return false;
    }
    if (!persistString(KeyTimezone, timezone)) {
        return false;
    }
    configuration_.time.timezone = timezone;
    return true;
}

bool ConfigurationService::setNtpServer1(const String& server) {
    if (!validateNtpServer(server)) {
        return false;
    }
    if (!persistString(KeyNtpServer1, server)) {
        return false;
    }
    configuration_.time.ntpServer1 = server;
    return true;
}

bool ConfigurationService::setNtpServer2(const String& server) {
    if (!validateNtpServer(server)) {
        return false;
    }
    if (!persistString(KeyNtpServer2, server)) {
        return false;
    }
    configuration_.time.ntpServer2 = server;
    return true;
}

bool ConfigurationService::setLocale(Locale locale) {
    if (locale != Locale::GermanGermany
        && locale != Locale::EnglishUnitedKingdom
        && locale != Locale::EnglishUnitedStates) {
        return false;
    }
    if (!persistUInt(KeyLocale, static_cast<uint32_t>(locale))) {
        return false;
    }
    configuration_.locale.locale = locale;
    return true;
}

bool ConfigurationService::setPresentationUnit(MeasurementType type, PresentationUnit unit) {
    if (!supportsPresentationUnit(type, unit)) {
        return false;
    }

    const char* key = nullptr;
    PresentationUnit* configuredUnit = nullptr;
    switch (type) {
        case MeasurementType::Temperature:
            key = KeyTemperatureUnit;
            configuredUnit = &configuration_.presentation.temperature;
            break;
        case MeasurementType::AtmosphericPressure:
            key = KeyPressureUnit;
            configuredUnit = &configuration_.presentation.atmosphericPressure;
            break;
        case MeasurementType::SolarCellTemperature:
            key = KeySolarCellTemperatureUnit;
            configuredUnit = &configuration_.presentation.solarCellTemperature;
            break;
        case MeasurementType::RainDetectorLevel:
            key = KeyRainDetectorLevelUnit;
            configuredUnit = &configuration_.presentation.rainDetectorLevel;
            break;
        default:
            return false;
    }

    if (!persistUInt(key, static_cast<uint32_t>(unit))) {
        return false;
    }
    *configuredUnit = unit;
    return true;
}

bool ConfigurationService::validateSensorSlot(const SensorSlotConfiguration& slot) const {
    if (!isValidSensorId(slot.slotId) || slot.slotId > MaxSensorSlotCount
        || slot.name.isEmpty() || slot.name.length() > MaxSensorSlotNameLength) {
        return false;
    }
    const SensorImplementationMetadata* metadata =
        SensorImplementationRegistry::find(slot.implementation);
    if (metadata == nullptr) return false;
    if (slot.schedule.acquisitionMode != metadata->defaultSchedule.acquisitionMode) return false;
    if (slot.schedule.enabled != slot.enabled) return false;
    if (slot.schedule.acquisitionMode == AcquisitionMode::Periodic) {
        if (slot.schedule.sampleIntervalMs == 0
            || slot.schedule.sampleIntervalMs > 0x7FFFFFFFUL) return false;
    } else if (slot.schedule.acquisitionMode == AcquisitionMode::EventOnly) {
        if (slot.schedule.sampleIntervalMs != 0) return false;
    } else {
        return false;
    }
    if (slot.implementation == SensorImplementation::None) {
        return slot.hardware.kind == HardwareResourceKind::None;
    }
    if (metadata->interfaceKind == HardwareInterfaceKind::Simulation) {
        return slot.hardware.kind == HardwareResourceKind::None;
    }
    if (BoardCapabilities::current().validate(metadata->interfaceKind, slot.hardware,
            metadata->requiredGpioCapabilities)
        != HardwareResourceValidationResult::Valid) return false;
    if (slot.implementation == SensorImplementation::AM2302) {
        return slot.hardware.kind == HardwareResourceKind::GPIO
            && slot.implementationConfiguration.am2302.gpio.number == slot.hardware.gpio.number;
    }
    if (slot.implementation == SensorImplementation::RainGauge) {
        return slot.hardware.kind == HardwareResourceKind::GPIO
            && slot.implementationConfiguration.rainGauge.gpio.number == slot.hardware.gpio.number
            && std::isfinite(slot.implementationConfiguration.rainGauge.millimetersPerTip)
            && slot.implementationConfiguration.rainGauge.millimetersPerTip > 0.0F
            && slot.implementationConfiguration.rainGauge.millimetersPerTip <= 100.0F
            && slot.implementationConfiguration.rainGauge.debounceMs >= 1
            && slot.implementationConfiguration.rainGauge.debounceMs <= 5000;
    }
    if (slot.implementation == SensorImplementation::BME280) {
        return slot.hardware.kind == HardwareResourceKind::I2C
            && slot.implementationConfiguration.bme280.i2c.bus == slot.hardware.i2c.bus
            && slot.implementationConfiguration.bme280.i2c.address == slot.hardware.i2c.address
            && (slot.hardware.i2c.address == 0x76 || slot.hardware.i2c.address == 0x77);
    }
    if (slot.implementation == SensorImplementation::SHT4x) {
        return slot.hardware.kind == HardwareResourceKind::I2C
            && slot.implementationConfiguration.sht4x.i2c.bus == slot.hardware.i2c.bus
            && slot.implementationConfiguration.sht4x.i2c.address == slot.hardware.i2c.address
            && slot.hardware.i2c.address == 0x44;
    }
    if (slot.implementation == SensorImplementation::SHTC3) {
        return slot.hardware.kind == HardwareResourceKind::I2C
            && slot.implementationConfiguration.shtc3.i2c.bus == slot.hardware.i2c.bus
            && slot.implementationConfiguration.shtc3.i2c.address == slot.hardware.i2c.address
            && slot.hardware.i2c.address == 0x70;
    }
    return true;
}

bool ConfigurationService::validateSensorSlots(const SensorSlotConfiguration* slots) const {
    for (size_t index = 0; index < MaxSensorSlotCount; ++index) {
        if (slots[index].slotId != index + 1 || !validateSensorSlot(slots[index])) return false;
    }
    return true;
}

bool ConfigurationService::validateActuatorSlot(
    const ActuatorSlotConfiguration& slot) const {
    if (!isValidActuatorId(slot.slotId) || slot.slotId > MaxActuatorSlotCount
        || slot.name.isEmpty() || slot.name.length() > MaxActuatorSlotNameLength) {
        return false;
    }
    const ActuatorImplementationMetadata* metadata =
        ActuatorImplementationRegistry::find(slot.implementation);
    if (metadata == nullptr) return false;
    if (slot.implementation == ActuatorImplementation::None) {
        return slot.hardware.kind == HardwareResourceKind::None;
    }
    return BoardCapabilities::current().validate(
        metadata->interfaceKind,
        slot.hardware,
        metadata->requiredGpioCapabilities) == HardwareResourceValidationResult::Valid;
}

bool ConfigurationService::validateActuatorSlots(
    const ActuatorSlotConfiguration* slots) const {
    for (size_t index = 0; index < MaxActuatorSlotCount; ++index) {
        if (slots[index].slotId != index + 1 || !validateActuatorSlot(slots[index])) {
            return false;
        }
    }
    return true;
}

bool ConfigurationService::validateControllerSlot(
    const ControllerSlotConfiguration& slot,
    const SensorSlotConfiguration* sensorSlots,
    const ActuatorSlotConfiguration* actuatorSlots) const {
    if (!isValidControllerId(slot.slotId)
        || slot.slotId > MaxControllerSlotCount
        || slot.name.isEmpty()
        || slot.name.length() > MaxControllerSlotNameLength) {
        return false;
    }
    const ControllerImplementationMetadata* metadata =
        ControllerImplementationRegistry::find(slot.implementation);
    if (metadata == nullptr) return false;
    if (slot.implementation == ControllerImplementation::None) return true;
    if (!slot.enabled) return true;

    if (slot.implementation == ControllerImplementation::Blink) {
        const BlinkControllerConfiguration& blink = slot.implementationConfiguration.blink;
        if (blink.onDurationMs == 0 || blink.onDurationMs > INT32_MAX
            || blink.offDurationMs == 0 || blink.offDurationMs > INT32_MAX) {
            return false;
        }
    } else if (slot.implementation == ControllerImplementation::Threshold) {
        const ThresholdControllerConfiguration& threshold =
            slot.implementationConfiguration.threshold;
        if (!std::isfinite(threshold.onThreshold)
            || !std::isfinite(threshold.offThreshold)
            || threshold.offThreshold >= threshold.onThreshold
            || threshold.maxMeasurementAgeMs == 0
            || threshold.maxMeasurementAgeMs > INT32_MAX) {
            return false;
        }
        if (sensorSlots == nullptr
            || !isValidSensorId(threshold.source.sensorId)
            || threshold.source.sensorId > MaxSensorSlotCount) {
            return false;
        }
        const SensorSlotConfiguration& source =
            sensorSlots[threshold.source.sensorId - 1];
        if (source.slotId != threshold.source.sensorId
            || !source.enabled
            || source.implementation == SensorImplementation::None) {
            return false;
        }
        const SensorImplementationMetadata* sensorMetadata =
            SensorImplementationRegistry::find(source.implementation);
        bool supportsType = false;
        if (sensorMetadata != nullptr) {
            for (size_t index = 0; index < sensorMetadata->measurementTypeCount; ++index) {
                if (sensorMetadata->measurementTypes[index]
                    == threshold.source.measurementType) {
                    supportsType = true;
                    break;
                }
            }
        }
        const MeasurementTypeMetadata& measurementMetadata =
            measurementTypeMetadata(threshold.source.measurementType);
        if (!supportsType
            || measurementMetadata.expectedValueKind != ValueKind::FloatingPoint
            || measurementMetadata.semantics != MeasurementSemantics::State) {
            return false;
        }
    } else {
        return false;
    }

    ActuatorId targetActuatorId = InvalidActuatorId;
    if (!configuredControllerTargetActuatorId(slot, targetActuatorId)) return false;
    if (!isValidActuatorId(targetActuatorId)
        || targetActuatorId > MaxActuatorSlotCount) {
        return false;
    }
    if (actuatorSlots == nullptr) return false;
    const ActuatorSlotConfiguration& target = actuatorSlots[targetActuatorId - 1];
    if (target.slotId != targetActuatorId
        || !target.enabled
        || target.implementation == ActuatorImplementation::None) {
        return false;
    }
    const ActuatorImplementationMetadata* actuatorMetadata =
        ActuatorImplementationRegistry::find(target.implementation);
    return actuatorMetadata != nullptr
        && hasActuatorCapability(
            actuatorMetadata->capabilities,
            metadata->requiredActuatorCapabilities);
}

bool ConfigurationService::validateControllerSlots(
    const ControllerSlotConfiguration* controllerSlots,
    const SensorSlotConfiguration* sensorSlots,
    const ActuatorSlotConfiguration* actuatorSlots) const {
    if (controllerSlots == nullptr || sensorSlots == nullptr || actuatorSlots == nullptr) {
        return false;
    }
    for (size_t index = 0; index < MaxControllerSlotCount; ++index) {
        if (controllerSlots[index].slotId != index + 1
            || !validateControllerSlot(
                controllerSlots[index], sensorSlots, actuatorSlots)) {
            return false;
        }
    }
    bool claimedTargets[MaxActuatorSlotCount + 1] = {};
    for (size_t index = 0; index < MaxControllerSlotCount; ++index) {
        const ControllerSlotConfiguration& slot = controllerSlots[index];
        if (!slot.enabled || slot.implementation == ControllerImplementation::None) {
            continue;
        }
        ActuatorId targetActuatorId = InvalidActuatorId;
        if (!configuredControllerTargetActuatorId(slot, targetActuatorId)
            || !isValidActuatorId(targetActuatorId)
            || targetActuatorId > MaxActuatorSlotCount
            || claimedTargets[targetActuatorId]) {
            return false;
        }
        claimedTargets[targetActuatorId] = true;
    }
    return true;
}

bool ConfigurationService::validateHardwareOccupancy(
    const SensorSlotConfiguration* sensorSlots,
    const ActuatorSlotConfiguration* actuatorSlots) const {
    HardwareResourceClaim claims[MaxSensorSlotCount + MaxActuatorSlotCount];
    size_t claimIndex = 0;
    for (size_t index = 0; index < MaxSensorSlotCount; ++index) {
        claims[claimIndex].active = sensorSlots[index].enabled
            && sensorSlots[index].implementation != SensorImplementation::None;
        claims[claimIndex].assignment = sensorSlots[index].hardware;
        ++claimIndex;
    }
    for (size_t index = 0; index < MaxActuatorSlotCount; ++index) {
        claims[claimIndex].active = actuatorSlots[index].enabled
            && actuatorSlots[index].implementation != ActuatorImplementation::None;
        claims[claimIndex].assignment = actuatorSlots[index].hardware;
        ++claimIndex;
    }
    return validateExclusiveHardwareResourceOccupancy(claims, claimIndex);
}

bool ConfigurationService::persistSensorSlot(const SensorSlotConfiguration& slot) {
    const SensorImplementationMetadata* metadata = SensorImplementationRegistry::find(slot.implementation);
    if (metadata == nullptr) return false;
    const SensorId id = slot.slotId;
    return persistUInt(sensorKey(id, "id").c_str(), id)
        && persistUInt(sensorKey(id, "en").c_str(), slot.enabled ? 1 : 0)
        && persistString(sensorKey(id, "name").c_str(), slot.name)
        && persistString(sensorKey(id, "impl").c_str(), metadata->stableId)
        && persistUInt(sensorKey(id, "mode").c_str(),
            slot.schedule.acquisitionMode == AcquisitionMode::Periodic ? 1 : 0)
        && persistUInt(sensorKey(id, "int").c_str(), slot.schedule.sampleIntervalMs)
        && persistUInt(sensorKey(id, "gpio").c_str(),
            slot.hardware.kind == HardwareResourceKind::GPIO ? slot.hardware.gpio.number : 0)
        && persistUInt(sensorKey(id, "i2caddr").c_str(),
            slot.hardware.kind == HardwareResourceKind::I2C ? slot.hardware.i2c.address : 0x76)
        && persistUInt(sensorKey(id, "i2cbus").c_str(),
            slot.hardware.kind == HardwareResourceKind::I2C
                ? static_cast<uint32_t>(slot.hardware.i2c.bus)
                : static_cast<uint32_t>(I2CBus::I2C0))
        && persistFloat(sensorKey(id, "rgmm").c_str(),
            slot.implementationConfiguration.rainGauge.millimetersPerTip)
        && persistUInt(sensorKey(id, "rgdeb").c_str(),
            slot.implementationConfiguration.rainGauge.debounceMs);
}

bool ConfigurationService::setSensorSlotConfiguration(const SensorSlotConfiguration& slot) {
    if (!isValidSensorId(slot.slotId) || slot.slotId > MaxSensorSlotCount) return false;
    SensorSlotConfiguration candidate[MaxSensorSlotCount];
    for (size_t index = 0; index < MaxSensorSlotCount; ++index) {
        candidate[index] = configuration_.sensorSlots[index];
    }
    candidate[slot.slotId - 1] = slot;
    if (!validateSensorSlots(candidate)
        || !validateHardwareOccupancy(candidate, configuration_.actuatorSlots)
        || !validateControllerSlots(
            configuration_.controllerSlots, candidate, configuration_.actuatorSlots)
        || !persistSensorSlot(slot)) return false;
    configuration_.sensorSlots[slot.slotId - 1] = slot;
    return true;
}

bool ConfigurationService::persistActuatorSlot(const ActuatorSlotConfiguration& slot) {
    const ActuatorImplementationMetadata* metadata =
        ActuatorImplementationRegistry::find(slot.implementation);
    if (metadata == nullptr) return false;
    const ActuatorId id = slot.slotId;
    return persistUInt(actuatorKey(id, "id").c_str(), id)
        && persistUInt(actuatorKey(id, "en").c_str(), slot.enabled ? 1 : 0)
        && persistString(actuatorKey(id, "name").c_str(), slot.name)
        && persistString(actuatorKey(id, "impl").c_str(), metadata->stableId)
        && persistUInt(actuatorKey(id, "gpio").c_str(),
            slot.hardware.kind == HardwareResourceKind::GPIO ? slot.hardware.gpio.number : 0);
}

bool ConfigurationService::setActuatorSlotConfiguration(
    const ActuatorSlotConfiguration& slot) {
    if (!isValidActuatorId(slot.slotId) || slot.slotId > MaxActuatorSlotCount) return false;
    ActuatorSlotConfiguration candidate[MaxActuatorSlotCount];
    for (size_t index = 0; index < MaxActuatorSlotCount; ++index) {
        candidate[index] = configuration_.actuatorSlots[index];
    }
    candidate[slot.slotId - 1] = slot;
    if (!validateActuatorSlots(candidate)
        || !validateHardwareOccupancy(configuration_.sensorSlots, candidate)
        || !validateControllerSlots(
            configuration_.controllerSlots, configuration_.sensorSlots, candidate)
        || !persistActuatorSlot(slot)) return false;
    configuration_.actuatorSlots[slot.slotId - 1] = slot;
    return true;
}

bool ConfigurationService::persistControllerSlot(
    const ControllerSlotConfiguration& slot) {
    const ControllerImplementationMetadata* metadata =
        ControllerImplementationRegistry::find(slot.implementation);
    if (metadata == nullptr) return false;
    const ControllerId id = slot.slotId;
    const BlinkControllerConfiguration& blink = slot.implementationConfiguration.blink;
    const ThresholdControllerConfiguration& threshold =
        slot.implementationConfiguration.threshold;
    return persistUInt(controllerKey(id, "id").c_str(), id)
        && persistUInt(controllerKey(id, "en").c_str(), slot.enabled ? 1 : 0)
        && persistString(controllerKey(id, "name").c_str(), slot.name)
        && persistString(controllerKey(id, "impl").c_str(), metadata->stableId)
        && persistUInt(controllerKey(id, "act").c_str(), blink.targetActuatorId)
        && persistUInt(controllerKey(id, "onms").c_str(), blink.onDurationMs)
        && persistUInt(controllerKey(id, "offms").c_str(), blink.offDurationMs)
        && persistUInt(controllerKey(id, "src").c_str(), threshold.source.sensorId)
        && persistString(controllerKey(id, "type").c_str(),
            measurementTypeStableId(threshold.source.measurementType))
        && persistUInt(controllerKey(id, "tact").c_str(), threshold.targetActuatorId)
        && persistFloat(controllerKey(id, "on").c_str(), threshold.onThreshold)
        && persistFloat(controllerKey(id, "off").c_str(), threshold.offThreshold)
        && persistUInt(controllerKey(id, "age").c_str(),
            threshold.maxMeasurementAgeMs);
}

bool ConfigurationService::setControllerSlotConfiguration(
    const ControllerSlotConfiguration& slot) {
    if (!isValidControllerId(slot.slotId) || slot.slotId > MaxControllerSlotCount) {
        return false;
    }
    ControllerSlotConfiguration candidate[MaxControllerSlotCount];
    for (size_t index = 0; index < MaxControllerSlotCount; ++index) {
        candidate[index] = configuration_.controllerSlots[index];
    }
    candidate[slot.slotId - 1] = slot;
    if (!validateControllerSlots(
            candidate, configuration_.sensorSlots, configuration_.actuatorSlots)
        || !persistControllerSlot(slot)) {
        return false;
    }
    configuration_.controllerSlots[slot.slotId - 1] = slot;
    return true;
}

bool ConfigurationService::resetToDefaults() {
    ensurePreferencesStarted();
    if (!preferences_.clear()) {
        return false;
    }
    initializeDefaults();
    return true;
}

} // namespace EnvNode
