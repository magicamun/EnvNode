# RainDetector PCNT Lab-Firmware

Eigenständiger PoC für den digitalen 3,3-V-Ausgang eines ICM7555.
Keine Abhängigkeit von `firmware/`, keine EnvNode-Sensorintegration, kein
MQTT, keine Wetterinterpretation, Kapazitätsberechnung oder Heater-Steuerung.

## Projekt und Toolchain

Die produktive PlatformIO-Konfiguration verwendet `esp32dev`, Arduino und eine
ungepinnte `espressif32`-Plattform. Dieses Lab-Projekt fixiert die bei der
Implementierung installierte Plattform auf **7.0.1**. Der geprüfte Build nutzt
Arduino-ESP32 **2.0.17** (Paket `3.20017.241212+sha.dcc1105b`) und dessen
ESP-IDF **4.4.7**. Es benötigt keine zusätzlichen Bibliotheken.

Verwendet wird die klassische API aus `driver/pcnt.h`, PCNT Unit 0 / Channel 0:
`pcnt_unit_config`, `pcnt_counter_clear`, `pcnt_counter_resume`,
`pcnt_counter_pause` und `pcnt_get_counter_value`.
Steigende Flanken inkrementieren; fallende Flanken werden ignoriert.
Filter und CPU-PCNT-Interrupts sind deaktiviert. Es gibt keine ISR,
kein `pulseIn()` und keine GPIO-Abfrageschleife.

Referenz: [Espressif PCNT API für ESP-IDF 4.4](https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32/api-reference/peripherals/pcnt.html).

## Anschluss und Konfiguration

- ICM7555 `RAIN_FREQ` (digital, 3,3 V) an **GPIO 27** des klassischen ESP32.
- Gemeinsame Masse zwischen RainDetector und ESP32 herstellen.
- Keine 5-V-Signale einspeisen. GPIO 27 muss auf dem verwendeten Aufbau frei sein.
- Heater separat unbeschaltet bzw. hardwareseitig ausgeschaltet halten;
  diese Firmware initialisiert keinen Heater-Ausgang.
- GPIO, Messfenster und Baudrate stehen zentral in `include/LabConfig.h`.
  GPIO 27 ist eine Lab-Vorgabe, keine Zuordnung aus einem EnvNode-Boardprofil.
- Getestetes Build-Ziel ist `esp32dev`, nicht ESP32-C3/S3 oder andere Varianten.

## Messung und Grenzen

Der Zähler wird bei pausierter Messung gelöscht, dann gestartet und nach
nominell **250.000 µs** angehalten. Währenddessen gibt `delay(1)` CPU-Zeit frei;
PCNT zählt unabhängig weiter. Die Dauer wird über `esp_timer_get_time()` direkt
vor Resume und direkt nach Pause erfasst. Die Rechnung lautet:

`frequency_hz = pulse_count * 1000000.0 / (end_us - start_us)`

Das reale Fenster ist durch Task-Scheduling etwas länger als 250 ms und nicht
hardwaregenau getaktet. Die Zeitmessung umfasst zusätzlich den kleinen Aufwand
der Resume-/Pause-Aufrufe; sie ist eine Näherung der tatsächlichen Zählzeit.
Eine konkrete Dauer am Zielgerät wurde noch nicht gemessen. Drei Nachkommastellen
der Ausgabe sind keine Genauigkeitszusage: die Zählauflösung liegt bei etwa 4 Hz
pro Puls; hinzu kommen Gate-Timing- und ESP32-Taktfehler.

Serial-Ausgabe und Reset passieren zwischen den Fenstern. Es gibt daher eine
kurze Messpause; Zeitstempelabstände sind nicht gleich der Zählfensterdauer.
Die Firmware misst Fensterfrequenzen, keinen lückenlosen Gesamtpulszähler.

Der 16-Bit-PCNT setzt beim oberen Limit von 32767 Pulsen auf null zurück.
Um dann keine falsche niedrige Frequenz auszugeben, wird ausschließlich das
High-Limit-Ereignis aktiviert. Dessen gelatchtes Raw-Interrupt-Bit wird nach
Pause geprüft und vor jedem Fenster gelöscht (`PCNT.int_raw` / `PCNT.int_clr`,
klassische ESP32-Register). Dafür wird kein CPU-Interrupt aktiviert.
Ein Überlauf ergibt `nan` statt einer Frequenz; `pulse_count` ist dann nur der
Restzählerstand. Bei 250 ms liegt die Grenze ungefähr bei 131 kHz, bei längeren
Fenstern entsprechend niedriger. Für einige 10 kHz reicht der Zähler aus.

## Serial-Format

115200 Baud, 8N1. Einmalig nach Initialisierung:

```csv
timestamp_ms,pulse_count,frequency_hz
```

Danach eine Zeile je Fenster mit dezimalem Punkt und drei Nachkommastellen.
`timestamp_ms` bezeichnet das Fensterende in Millisekunden seit Boot.
Beispielwerte (keine reale Hardwaremessung):

```csv
1250,10000,40000.000
1505,0,0.000
1760,123,nan
```

Ohne Flanken ist das Ergebnis null; ein fehlendes Signal lässt sich daraus
nicht von einem stillstehenden Oszillator unterscheiden. Bei einem API-Fehler
erscheint `# error,<operation>,<ESP-Fehlername>` und die Messung stoppt bis Reset.
ESP32-ROM-Bootmeldungen können vor dem CSV-Header erscheinen. Für Auswertung
erst ab dem Header lesen und Fehlerzeilen bzw. `nan` behandeln.

## Build, Flash und Test

Vom Repository-Root aus (falls `pio` nicht im PATH liegt:
`~/.platformio/penv/bin/pio` verwenden):

```sh
pio run -d lab/RainDetector-PCNT
pio device list
pio run -d lab/RainDetector-PCNT -t upload --upload-port /dev/cu.usbserial-0001
pio device monitor -p /dev/cu.usbserial-0001 -b 115200 --filter direct
```

Den Beispielport durch den tatsächlichen ESP32-Port ersetzen. Flashen ersetzt
die aktuell auf diesem ESP32 laufende Firmware. Nach Öffnen des Monitors bei
Bedarf Reset drücken, um den CSV-Header zu erfassen.

Zunächst Signal und gemeinsame Masse verbinden, dann die Frequenz mit
Oszilloskop/Frequenzzähler vergleichen. Optional einen bekannten 3,3-V-Takt
einspeisen: 40 kHz sollten bei ungefähr 250 ms ungefähr 10000 Pulse ergeben,
unabhängig vom Tastverhältnis (innerhalb der PCNT-Eingangsgrenzen).
Mit auf GND gelegtem Eingang müssen null Pulse erscheinen. Optional mit
150 kHz testen: die Frequenz muss `nan` werden. Den ICM7555-Ausgang vor
Einspeisung eines externen Testtakts trennen.

Build und Quellcodeprüfung sind erfolgt; Flashen und Hardwaretests sind noch
nicht durchgeführt worden.
