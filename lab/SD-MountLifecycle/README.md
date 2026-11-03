# SD mount/unmount comparison

EnvNode: see repository LICENSE.

Standalone ESP32 sketch without EnvNode, WiFi, MQTT or a writer task. It repeats
`SD.begin()` / `SD.end()` 1000 times using a live local `SPIClass`, as in the
reported logging path. It never opens or writes a file and never formats.
Use an existing FAT-formatted card and verify wiring first. Default pins are
SCK18/MISO19/MOSI23/CS5 (EnvNode ESP32 Slot A); build flags can override them.

Build with `pio run -e arduino3` for PlatformIO 8 / Arduino 3.3.12, or
`pio run -e arduino2` for PlatformIO 7.0.1 / Arduino 2.0.17. Flash and monitor
explicitly on the intended test board. Both environments print the Arduino
version and per-cycle progress. A failed mount is reported, not counted as a
successful card access; a useful run needs `mounted=1`.

The original EnvNode crash is hardware-observed. This minimal sketch is a
candidate reproducer, not yet hardware-validated. The old use-after-free is
undefined behavior and may not crash with this simpler heap layout. A clean
run on the old core therefore does not disprove the source-level defect.
Arduino 3.3.12's corrected guard lifetime must be checked together with the
full EnvNode runtime test. This sketch does not validate logging, hotplug,
formatting or concurrent web access.
