#include <Adafruit_SHTC3.h>
#include <Arduino.h>
#include <Wire.h>

namespace {
constexpr uint8_t I2cSda = 21;
constexpr uint8_t I2cScl = 22;
constexpr uint8_t Shtc3Address = 0x70;
constexpr uint32_t SampleIntervalMs = 2000;

Adafruit_SHTC3 shtc3;
bool sensorReady = false;

bool addressResponds(uint8_t address) {
    Wire.beginTransmission(address);
    return Wire.endTransmission() == 0;
}
} // namespace

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("----------------------------------------");
    Serial.println("SHTC3 Hardware Test");
    Serial.println("----------------------------------------");

    Wire.begin(I2cSda, I2cScl);
    if (!addressResponds(Shtc3Address)) {
        Serial.println("ERROR: No I2C device detected at address 0x70.");
        return;
    }

    sensorReady = shtc3.begin(&Wire);
    if (!sensorReady) {
        Serial.println("ERROR: SHTC3 initialization failed.");
        return;
    }

    Serial.println("SHTC3 initialized at address 0x70.");
}
void loop() {
    if (!sensorReady) {
        delay(SampleIntervalMs);
        return;
    }

    sensors_event_t humidity = {};
    sensors_event_t temperature = {};
    if (!shtc3.getEvent(&humidity, &temperature)) {
        Serial.println("ERROR: Sensor read failed.");
        delay(SampleIntervalMs);
        return;
    }

    Serial.printf(
        "Temperature: %6.2f °C   Humidity: %6.2f %%RH\n",
        temperature.temperature,
        humidity.relative_humidity);
    delay(SampleIntervalMs);
}
