// EnvNode: see repository LICENSE.
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <esp_arduino_version.h>

// ESP32 EnvNode shared SPI, Slot A CS. Override with build_flags for other wiring.
#ifndef SD_TEST_CS
#define SD_TEST_CS 5
#endif
#ifndef SD_TEST_SCK
#define SD_TEST_SCK 18
#endif
#ifndef SD_TEST_MISO
#define SD_TEST_MISO 19
#endif
#ifndef SD_TEST_MOSI
#define SD_TEST_MOSI 23
#endif

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.printf("SD lifecycle test: Arduino %s; CS=%d SCK=%d MISO=%d MOSI=%d\n",
        ESP_ARDUINO_VERSION_STR, SD_TEST_CS, SD_TEST_SCK, SD_TEST_MISO, SD_TEST_MOSI);
}

void loop() {
    static unsigned cycle = 0;
    if (cycle >= 1000) { delay(1000); return; }
    ++cycle;
    SPIClass spi(VSPI);
    spi.begin(SD_TEST_SCK, SD_TEST_MISO, SD_TEST_MOSI, SD_TEST_CS);
    const bool mounted = SD.begin(SD_TEST_CS, spi, 1000000, "/sd", 1, false);
    Serial.printf("cycle=%u mounted=%u heap=%lu; calling SD.end()\n",
        cycle, static_cast<unsigned>(mounted), static_cast<unsigned long>(ESP.getFreeHeap()));
    // All SPI objects are alive here. On Arduino 2.0.17, sdcard_uninit() frees
    // its card before the AcquireSPI destructor reads card->spi.
    SD.end();
    spi.end();
    Serial.printf("cycle=%u end completed\n", cycle);
    delay(100);
}
