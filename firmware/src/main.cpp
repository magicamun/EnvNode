#include "Application.h"
#include "ConfigurationService.h"
#include "SerialLogger.h"
#include "WiFiService.h"
#include "WebService.h"
#include "MqttService.h"
#include "TimeService.h"
#include "ArduinoMonotonicClock.h"
#include "DiagnosticMeasurementSink.h"
#include "SensorManager.h"
#include "SimulatedTemperatureSensor.h"

using namespace WeatherStation;

static SerialLogger serialLogger;
static ConfigurationService configurationService;
static WiFiService wifiService(serialLogger, configurationService);
static TimeService timeService(serialLogger, configurationService, wifiService);
static MqttService mqttService(serialLogger, configurationService, wifiService);
static WebService webService(serialLogger, configurationService, wifiService);
static ArduinoMonotonicClock monotonicClock;
static DiagnosticMeasurementSink measurementSink(serialLogger);
static SensorManager sensorManager(timeService, monotonicClock, measurementSink);
static SimulatedTemperatureSensor simulatedTemperatureSensor(1, monotonicClock);
static Application app(serialLogger, configurationService, wifiService, webService, mqttService, timeService, sensorManager);

void setup() {
    const SensorRegistrationResult registrationResult = sensorManager.registerSensor(
        simulatedTemperatureSensor,
        SensorSchedule::periodic(5000));

    app.setup();

    if (registrationResult != SensorRegistrationResult::Registered) {
        serialLogger.printf(
            "Simulated temperature sensor registration failed: result=%d\n",
            static_cast<int>(registrationResult));
    }
}

void loop() {
    app.loop();
}
