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
static Application app(serialLogger, configurationService, wifiService, webService, mqttService, timeService, sensorManager, runtimeManager);

namespace {

SensorSlotConfiguration simulatedSlot(
    SensorId id,
    const char* name,
    SensorImplementation implementation) {
    SensorSlotConfiguration slot;
    slot.slotId = id;
    slot.enabled = true;
    slot.name = name;
    slot.implementation = implementation;
    const SensorImplementationMetadata* metadata = SensorImplementationRegistry::find(implementation);
    slot.schedule = metadata == nullptr ? SensorSchedule::eventOnly(false) : metadata->defaultSchedule;
    slot.schedule.enabled = slot.enabled;
    slot.hardware = HardwareResourceAssignment::none();
    return slot;
}

SensorSlotConfiguration am2302Slot(SensorId id, const char* name, GpioResource gpio) {
    SensorSlotConfiguration slot;
    slot.slotId = id;
    slot.enabled = true;
    slot.name = name;
    slot.implementation = SensorImplementation::AM2302;
    const SensorImplementationMetadata* metadata = SensorImplementationRegistry::find(slot.implementation);
    slot.schedule = metadata == nullptr ? SensorSchedule::eventOnly(false) : metadata->defaultSchedule;
    slot.schedule.enabled = slot.enabled;
    slot.hardware = HardwareResourceAssignment::gpioResource(gpio);
    slot.implementationConfiguration.am2302 = AM2302Configuration(gpio);
    return slot;
}

const SensorSlotConfiguration SensorSlots[] = {
    simulatedSlot(1, "Simulated Temperature", SensorImplementation::SimulatedTemperature),
    simulatedSlot(2, "Simulated Humidity", SensorImplementation::SimulatedHumidity),
    simulatedSlot(3, "Simulated Barometer", SensorImplementation::SimulatedPressure),
    am2302Slot(4, "Outside", GpioResource(27)),
};

class RuntimeSensorComposition {
public:
    RuntimeSensorComposition(IMonotonicClock& clock, ILogger& logger)
        : simulatedTemperature_(SensorSlots[0].slotId, clock)
        , simulatedHumidity_(SensorSlots[1].slotId, clock)
        , simulatedPressure_(SensorSlots[2].slotId, clock)
        , am2302_(SensorSlots[3].slotId,
            SensorSlots[3].implementationConfiguration.am2302.gpio.number, clock, logger) {
    }

    void registerSensors(SensorManager& manager, ILogger& logger) {
        registerSensor(manager, logger, SensorSlots[0], simulatedTemperature_);
        registerSensor(manager, logger, SensorSlots[1], simulatedHumidity_);
        registerSensor(manager, logger, SensorSlots[2], simulatedPressure_);
        registerSensor(manager, logger, SensorSlots[3], am2302_);
    }

private:
    static void registerSensor(
        SensorManager& manager,
        ILogger& logger,
        const SensorSlotConfiguration& slot,
        ISensor& sensor) {
        if (!slot.enabled) return;
        const SensorImplementationMetadata* implementation =
            SensorImplementationRegistry::find(slot.implementation);
        if (implementation == nullptr) {
            logger.printf("Sensor slot %u has an unknown implementation\n", slot.slotId);
            return;
        }
        const HardwareResourceValidationResult validation = BoardCapabilities::current().validate(
            implementation->interfaceKind, slot.hardware);
        if (validation != HardwareResourceValidationResult::Valid) {
            logger.printf("Sensor slot %u resource validation failed: result=%d\n",
                slot.slotId, static_cast<int>(validation));
            return;
        }
        SensorSchedule runtimeSchedule = slot.schedule;
        runtimeSchedule.enabled = slot.enabled;
        const SensorRegistrationResult result = manager.registerSensor(
            sensor,
            runtimeSchedule,
            SensorRegistrationMetadata(
                slot.name,
                slot.implementation,
                hardwareInterfaceKindName(implementation->interfaceKind),
                implementation->protocolDescription,
                implementation->configurationSchemaDescription,
                slot.hardware));
        if (result != SensorRegistrationResult::Registered) {
            logger.printf("Sensor slot %u (%s) registration failed: result=%d\n",
                slot.slotId, implementation->displayType, static_cast<int>(result));
        }
    }

    SimulatedTemperatureSensor simulatedTemperature_;
    SimulatedHumiditySensor simulatedHumidity_;
    SimulatedPressureSensor simulatedPressure_;
    AM2302Sensor am2302_;
};

RuntimeSensorComposition runtimeSensors(monotonicClock, serialLogger);

} // namespace

void setup() {
    runtimeSensors.registerSensors(sensorManager, serialLogger);

    app.setup();
}

void loop() {
    app.loop();
}
