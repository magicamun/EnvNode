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

using namespace WeatherStation;

static SerialLogger serialLogger;
static ConfigurationService configurationService;
static WiFiService wifiService(serialLogger, configurationService);
static TimeService timeService(serialLogger, configurationService, wifiService);
static MqttService mqttService(serialLogger, configurationService, wifiService);
static ArduinoMonotonicClock monotonicClock;
static MeasurementPublisher measurementPublisher(configurationService, timeService, mqttService);
static MeasurementSnapshotCache measurementSnapshotCache;
static SensorManager sensorManager(
    timeService,
    monotonicClock,
    measurementPublisher,
    measurementSnapshotCache);
static SensorFactory firstSensorFactory(monotonicClock, serialLogger);
static SensorFactory secondSensorFactory(monotonicClock, serialLogger);
static SensorRuntime sensorRuntime(
    configurationService,
    firstSensorFactory,
    secondSensorFactory,
    sensorManager);
static LocaleFormatter localeFormatter(configurationService);
static RuntimeManager runtimeManager(serialLogger, &sensorRuntime);
static HomeAssistantDiscoveryPublisher homeAssistantDiscoveryPublisher(
    serialLogger,
    configurationService,
    mqttService,
    sensorManager);
static OTAService otaService(serialLogger, runtimeManager);
static WebService webService(serialLogger, configurationService, wifiService, mqttService, timeService, localeFormatter, sensorManager, measurementSnapshotCache, homeAssistantDiscoveryPublisher, runtimeManager, otaService);
static Application app(serialLogger, configurationService, wifiService, webService, mqttService, timeService, sensorManager, runtimeManager, homeAssistantDiscoveryPublisher);

void setup() {
    configurationService.loadConfiguration();
    size_t activeSensorCount = 0;
    const char* sensorFailureReason = nullptr;
    const bool sensorsInitialized = sensorRuntime.initialize(
        activeSensorCount, sensorFailureReason);

    app.setup(true);
    if (!sensorsInitialized) {
        serialLogger.printf("Sensor runtime initialization failed: %s\n",
            sensorFailureReason == nullptr ? "unknown failure" : sensorFailureReason);
    }
}

void loop() {
    app.loop();
}
