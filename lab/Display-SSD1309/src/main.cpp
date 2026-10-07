#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

#include "LabConfig.h"

namespace {
// SSD1309 NONAME2 is a candidate, not an identification from the I2C ACK.
U8G2_SSD1309_128X64_NONAME2_F_HW_I2C display(
    U8G2_R0, U8X8_PIN_NONE, LabConfig::SclPin, LabConfig::SdaPin);

const char* const lines[] = {
    "EnvNode OLED Lab",
    "Level : 1234 l 56%",
    "Modus : Auto",
    "Status: Hysterese",
    "Ventil: Zisterne",
    "06.10.2026 12:34"
};
static_assert(sizeof(lines) / sizeof(lines[0]) == 6, "Expected six lines");

void renderTestPage() {
    display.clearBuffer();
    display.setFont(u8g2_font_t0_11_tf);
    display.setFontPosBaseline();
    for (size_t i = 0; i < 6; ++i) {
        const auto width = display.getStrWidth(lines[i]);
        Serial.printf("Line %u: width=%u/128 px, %s\n",
                      static_cast<unsigned>(i + 1),
                      static_cast<unsigned>(width), lines[i]);
        display.drawStr(0, (i + 1) * LabConfig::LineHeight, lines[i]);
    }
    display.sendBuffer();
}
}

void setup() {
    Serial.begin(LabConfig::SerialBaud);
    delay(500);
    Serial.printf("\nOLED Lab: SSD1309 NONAME2 candidate, SDA=%u SCL=%u, I2C=0x%02X\n",
                  LabConfig::SdaPin, LabConfig::SclPin, LabConfig::Address);
    if (!Wire.begin(LabConfig::SdaPin, LabConfig::SclPin, LabConfig::BusClockHz)) {
        Serial.println("ERROR: I2C initialization failed; reset after checking wiring.");
        return;
    }
    Wire.setTimeOut(100);
    Wire.beginTransmission(LabConfig::Address);
    const uint8_t result = Wire.endTransmission();
    if (result != 0) {
        Serial.printf("ERROR: No ACK at 0x%02X (I2C status %u); no display commands sent.\n",
                      LabConfig::Address, result);
        return;
    }
    Serial.println("I2C ACK received; this does not identify the controller.");
    // U8g2 expects the shifted address, unlike Wire.beginTransmission().
    display.setI2CAddress(LabConfig::Address << 1);
    display.setBusClock(LabConfig::BusClockHz);
    display.begin();
    display.setPowerSave(0);
    renderTestPage();
    Serial.println("Test page sent. Verify all six lines visually; all values are fixed examples.");
}

void loop() {
    delay(1000);
}
