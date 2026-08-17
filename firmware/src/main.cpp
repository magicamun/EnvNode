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
#include "GpioOnOffActuator.h"

using namespace EnvNode;

static SerialLogger serialLogger;
static const HardwareResourceAssignment temporaryLedHardware =
    HardwareResourceAssignment::gpioResource(GpioResource(16));
static GpioOnOffActuator temporaryLedActuator(temporaryLedHardware, serialLogger);
static I2CBusManager i2cBusManager(serialLogger);
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
static SensorFactory firstSensorFactory(monotonicClock, i2cBusManager, serialLogger);
static SensorFactory secondSensorFactory(monotonicClock, i2cBusManager, serialLogger);
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

// Temporary hardware smoke test. Remove after the GPIO OnOff actuator has been
// verified with an LED and series resistor on GPIO14.
static void runTemporaryLedActuatorSmokeTest() {
    const Configuration& configuration = configurationService.getConfiguration();
    for (size_t index = 0; index < MaxSensorSlotCount; ++index) {
        const SensorSlotConfiguration& slot = configuration.sensorSlots[index];
        if (slot.enabled
            && slot.implementation != SensorImplementation::None
            && exclusiveHardwareResourceConflict(slot.hardware, temporaryLedHardware)) {
            serialLogger.printf(
                "Temporary LED actuator smoke test skipped: GPIO14 conflicts with sensor %u\n",
                slot.slotId);
            return;
        }
    }

    const ActuatorOperationResult initializationResult = temporaryLedActuator.begin();
    if (initializationResult != ActuatorOperationResult::Completed) {
        serialLogger.printf(
            "Temporary LED actuator smoke test initialization failed: result=%u\n",
            static_cast<unsigned int>(initializationResult));
        return;
    }

    serialLogger.println("Temporary LED actuator smoke test initialized successfully");
    if (temporaryLedActuator.setState(OnOffState::On)
        != ActuatorOperationResult::Completed) {
        serialLogger.println("Temporary LED actuator smoke test failed to switch On");
        return;
    }
    serialLogger.println("Temporary LED actuator smoke test: On for 3000 ms");
    delay(3000);

    if (temporaryLedActuator.setState(OnOffState::Off)
        != ActuatorOperationResult::Completed) {
        serialLogger.println("Temporary LED actuator smoke test failed to switch Off");
        return;
    }
    serialLogger.println("Temporary LED actuator smoke test: Off");
}

void setup() {
    i2cBusManager.begin();
    configurationService.loadConfiguration();
    size_t activeSensorCount = 0;
    const char* sensorFailureReason = nullptr;
    const bool sensorsInitialized = sensorRuntime.initialize(
        activeSensorCount, sensorFailureReason);

    app.setup(true);
    runTemporaryLedActuatorSmokeTest();
    if (!sensorsInitialized) {
        serialLogger.printf("Sensor runtime initialization failed: %s\n",
            sensorFailureReason == nullptr ? "unknown failure" : sensorFailureReason);
    }
}

void loop() {
    app.loop();
}
