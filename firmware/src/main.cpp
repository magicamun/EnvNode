#include "Application.h"
#include "ConfigurationService.h"
#include "SerialLogger.h"
#include "WiFiService.h"
#include "WebService.h"
#include "MqttService.h"
#include "TimeService.h"
#include "ArduinoMonotonicClock.h"
#include "MeasurementPublisher.h"
#include "SensorManager.h"
#include "SimulatedTemperatureSensor.h"
#include "SimulatedHumiditySensor.h"
#include "SimulatedPressureSensor.h"
#include "AM2302Sensor.h"
#include "LocaleFormatter.h"
#include "RuntimeManager.h"
#include "OTAService.h"
#include "HardwareResources.h"
#include "SensorImplementationRegistry.h"
#include "SensorSlotConfiguration.h"
#include "SensorFactory.h"
#include "SensorRuntime.h"
#include "HomeAssistantDiscoveryPublisher.h"
#include "MeasurementSnapshotCache.h"
#include "I2CBusManager.h"
#include "ActuatorFactory.h"
#include "ActuatorRuntime.h"
#include "ActuatorMqttAdapter.h"
#include "ActuatorStatePublisher.h"
#include "ControllerFactory.h"
#include "ControllerRuntime.h"
#include "ControllerMqttAdapter.h"
#include "ControllerStatePublisher.h"
#include "MqttMessageRouter.h"
#include "MqttDescriptionPublisher.h"
#include "StructuredLogger.h"
#include "RecentLogStore.h"
#include "SystemLogTimeProvider.h"
#include "BoardIdentityEeprom24AA025E48.h"
#include "BoardIdentityResolver.h"

using namespace EnvNode;

static ArduinoMonotonicClock monotonicClock;
static SystemLogTimeProvider logTimeProvider(monotonicClock);
static RecentLogStore recentLogStore;
static SerialLogger serialLogSink;
static StructuredLogger logger(recentLogStore, logTimeProvider, serialLogSink);
static ActuatorFactory actuatorFactory(logger);
static ActuatorRuntime actuatorRuntime(actuatorFactory, logger);
static I2CBusManager i2cBusManager(logger);
static BoardIdentityEeprom24AA025E48 boardIdentityEeprom(i2cBusManager);
static BoardIdentityStore boardIdentityStore(boardIdentityEeprom);
static BoardIdentityResolver boardIdentityResolver(
    boardIdentityStore, buildFallbackBoardProfileId());
static ConfigurationService configurationService;
static WiFiService wifiService(logger, configurationService);
static TimeService timeService(logger, configurationService, wifiService);
static MqttService mqttService(logger, configurationService, wifiService);
static ActuatorMqttAdapter actuatorMqttAdapter(
    logger, configurationService, mqttService, actuatorRuntime);
static ActuatorStatePublisher actuatorStatePublisher(
    logger, configurationService, mqttService, actuatorRuntime);
static MeasurementSnapshotCache measurementSnapshotCache;
static ControllerFactory controllerFactory(
    measurementSnapshotCache, actuatorRuntime, monotonicClock, logger);
static ControllerRuntime controllerRuntime(controllerFactory, logger);
static MeasurementPublisher measurementPublisher(configurationService, timeService, mqttService);
static SensorManager sensorManager(
    timeService,
    monotonicClock,
    measurementPublisher,
    measurementSnapshotCache);
static SensorFactory firstSensorFactory(monotonicClock, i2cBusManager, logger);
static SensorFactory secondSensorFactory(monotonicClock, i2cBusManager, logger);
static SensorRuntime sensorRuntime(
    configurationService,
    firstSensorFactory,
    secondSensorFactory,
    sensorManager);
static LocaleFormatter localeFormatter(configurationService);
static RuntimeManager runtimeManager(
    logger, &sensorRuntime, &actuatorRuntime, &controllerRuntime);
static ControllerMqttAdapter controllerMqttAdapter(
    logger, configurationService, mqttService, controllerRuntime, runtimeManager);
static MqttMessageRouter mqttMessageRouter(
    mqttService, actuatorMqttAdapter, controllerMqttAdapter);
static ControllerStatePublisher controllerStatePublisher(
    logger, configurationService, mqttService, controllerRuntime);
static MqttDescriptionPublisher mqttDescriptionPublisher(
    logger, configurationService, mqttService, monotonicClock);
static HomeAssistantDiscoveryPublisher homeAssistantDiscoveryPublisher(
    logger,
    configurationService,
    mqttService,
    sensorManager,
    actuatorRuntime);
static OTAService otaService(logger, runtimeManager);
static WebService webService(logger, configurationService, wifiService, mqttService, timeService, localeFormatter, sensorManager, actuatorRuntime, controllerRuntime, measurementSnapshotCache, recentLogStore, homeAssistantDiscoveryPublisher, runtimeManager, otaService, i2cBusManager, boardIdentityResolver.resolution());
static Application app(logger, configurationService, wifiService, webService, mqttService, timeService, sensorManager, actuatorRuntime, controllerRuntime, runtimeManager, homeAssistantDiscoveryPublisher, mqttMessageRouter, actuatorMqttAdapter, actuatorStatePublisher, controllerMqttAdapter, controllerStatePublisher, mqttDescriptionPublisher);
static bool normalRuntimeStarted = false;

void setup() {
    logger.begin(115200);
    delay(500);
    i2cBusManager.beginIdentityBus();
    const BoardIdentityResolution& identityResolution = boardIdentityResolver.resolve();
    logger.infof(
        "Board identity source=%s status=%s profile=%u revision=%u.%u serial=%lu",
        boardIdentitySourceName(identityResolution.source),
        boardIdentityStatusName(identityResolution.recordStatus),
        static_cast<unsigned int>(identityResolution.identity.profileId),
        identityResolution.identity.revision.major,
        identityResolution.identity.revision.minor,
        static_cast<unsigned long>(identityResolution.identity.serialNumber));
    if (!identityResolution.normalRuntimeAllowed
        || !selectCurrentBoardProfile(identityResolution.identity.profileId)) {
        logger.error("Board identity unsupported; normal runtime initialization stopped");
        return;
    }

    i2cBusManager.begin();
    configurationService.loadConfiguration();
    size_t activeSensorCount = 0;
    const char* sensorFailureReason = nullptr;
    const bool sensorsInitialized = sensorRuntime.initialize(
        activeSensorCount, sensorFailureReason);
    app.setup(true);
    if (!sensorsInitialized) {
        logger.errorf("Sensor runtime initialization failed: %s",
            sensorFailureReason == nullptr ? "unknown failure" : sensorFailureReason);
    }
    normalRuntimeStarted = true;
}

void loop() {
    if (normalRuntimeStarted) {
        app.loop();
    }
}
