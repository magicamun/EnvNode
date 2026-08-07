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
#include "LocaleFormatter.h"
#include "RuntimeManager.h"
#include "OTAService.h"

using namespace WeatherStation;

static SerialLogger serialLogger;
static ConfigurationService configurationService;
static WiFiService wifiService(serialLogger, configurationService);
static TimeService timeService(serialLogger, configurationService, wifiService);
static MqttService mqttService(serialLogger, configurationService, wifiService);
static ArduinoMonotonicClock monotonicClock;
static MeasurementPublisher measurementPublisher(configurationService, timeService, mqttService);
static SensorManager sensorManager(timeService, monotonicClock, measurementPublisher);
static LocaleFormatter localeFormatter(configurationService);
static RuntimeManager runtimeManager(serialLogger);
static OTAService otaService(serialLogger, runtimeManager);
static WebService webService(serialLogger, configurationService, wifiService, mqttService, timeService, localeFormatter, sensorManager, runtimeManager, otaService);
static SimulatedTemperatureSensor simulatedTemperatureSensor(1, monotonicClock);
static SimulatedHumiditySensor simulatedHumiditySensor(2, monotonicClock);
static SimulatedPressureSensor simulatedPressureSensor(3, monotonicClock);
static Application app(serialLogger, configurationService, wifiService, webService, mqttService, timeService, sensorManager, runtimeManager);

void setup() {
    const SensorRegistrationResult temperatureRegistrationResult = sensorManager.registerSensor(
        simulatedTemperatureSensor,
        SensorSchedule::periodic(5000));
    const SensorRegistrationResult humidityRegistrationResult = sensorManager.registerSensor(
        simulatedHumiditySensor,
        SensorSchedule::periodic(5000));
    const SensorRegistrationResult pressureRegistrationResult = sensorManager.registerSensor(
        simulatedPressureSensor,
        SensorSchedule::periodic(10000));

    app.setup();

    if (temperatureRegistrationResult != SensorRegistrationResult::Registered) {
        serialLogger.printf(
            "Simulated temperature sensor registration failed: result=%d\n",
            static_cast<int>(temperatureRegistrationResult));
    }
    if (humidityRegistrationResult != SensorRegistrationResult::Registered) {
        serialLogger.printf(
            "Simulated humidity sensor registration failed: result=%d\n",
            static_cast<int>(humidityRegistrationResult));
    }
    if (pressureRegistrationResult != SensorRegistrationResult::Registered) {
        serialLogger.printf(
            "Simulated pressure sensor registration failed: result=%d\n",
            static_cast<int>(pressureRegistrationResult));
    }
}

void loop() {
    app.loop();
}
