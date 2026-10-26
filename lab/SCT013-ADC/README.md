# SCT013Head Lab - EnvNode Mini

Standalone PlatformIO/Arduino PoC for a classic ESP32 DevKit, GPIO35 (ADC1_CH7), exposed by Mini port ADC2, is the initial default.
Mini port ADC1 exposes GPIO34 (ADC1_CH6). Both ports use ESP32 ADC unit 1.
No production firmware dependencies, WiFi, MQTT, or automatic flashing.

## Wiring

Use the existing SensorHead circuit: R1/R2/R3/R4 all 10 kOhm.
R2/R3 divide 3.3 V to VBIAS; C2 10 uF connects VBIAS to GND.
Sensor sleeve to VBIAS, tip through R1 to ADC; R4 from ADC to VBIAS.
ADC to GPIO35 through Mini port ADC2, common GND and 3.3 V. On the assembled Head, populate C1 10 nF.
Only voltage-output sensors with integrated burden: 5 A / 0.333 V or 5 A / 1 V.
Clamp around ONE insulated conductor, not both supply and return together.
Use an enclosed mains test lead/adaptor; mains never belongs on the breadboard.

## Build / upload / monitor

From this directory:

```
pio run
pio device list
pio run -t upload --upload-port /dev/cu.YOUR_DEVICE
pio device monitor --port /dev/cu.YOUR_DEVICE --baud 115200
```

The port is deliberately not fixed. Close other monitors before upload.
Commands require Enter and are processed between one-second measurement windows:

- `sensor 333`: 5 A / 0.333 V, initial default.
- `sensor 1000`: 5 A / 1 V.
- `pin 34` / `pin 35`: select ADC input; selection persists in NVS.
- `status`: selected pin, profile and scale.
- `cal 1.05`: multiplicative current correction (decimal point), default 1.
- `help`: commands and CSV header.

Profile and correction persist across reset in NVS. Changing profile resets correction.
Never assume the sensor is detected automatically. Verify its label and select the profile.

## First test with the 60 W incandescent lamp

1. Lamp OFF: check offset near 1650 mV; record AC RMS and current as noise baseline.
   Nonzero idle current is expected, especially without C1; nothing is hidden or subtracted.
2. Select the correct sensor profile.
3. Lamp ON: compare several stable readings after the switch-on transient.
   Nominally 60 W / 230 V is about 0.261 A for this resistive load.
   Nameplate power and assumed voltage are not a precision calibration reference.
4. Switch OFF and confirm return towards the baseline.
5. Repeat with the other sensor and its profile.

At 0.261 A the ideal ADC AC RMS is about 8.7 mV (0.333-V sensor) or
26.1 mV (1-V sensor). The former is small relative to ESP32 ADC noise/nonlinearity.
Both are only about 5.2% of the sensor's 5-A rating; precision is not assured here.

## Output and method

CSV: uptime ms, measured offset mV, AC RMS mV, current A, minimum mV,
maximum mV, sample count, late-sample count, flags. 2000 calibrated samples
at nominal 2 kHz per window; subtract each window's mean using Welford variance.
A raw ADC sample is converted once with esp_adc_cal (not a second ADC reading).
Calibration source is printed at boot; default-Vref fallback is approximate.

Nominal factors with divider gain 0.5: 30.030 A/V or 10 A/V.
Optional correction scales the result; no automatic lamp-based calibration.
No real-power output: that would require simultaneous voltage/phase measurement.

Flags: BIAS_CHECK outside 1.4–1.9 V mean; ADC_RANGE outside the recommended
0.15–2.45 V range; POSSIBLE_CLIP near the rails; TIMING for sampling overruns.
These are diagnostics, not electrical protection or definitive clipping detection.
Without C1, high-frequency noise can alias; this is a bring-up test, not a metering instrument.

## Calculation test

Run `c++ -std=c++11 -Iinclude tests/rms_test.cpp -o /tmp/sct-rms-test && /tmp/sct-rms-test`.
Checks DC rejection and exact sinusoidal RMS at the two lamp-level signals and full-scale input.

## Raw ADC diagnostics

After flashing, use `diag on` for additional lines after each unchanged CSV row:

- `RAW`: mean and AC RMS in ADC counts, minimum/maximum, nearest-rank
  percentiles p01/p50/p99, and counts at raw codes 0 and 4095.
- `NOISE`: number of samples more than 20 or 50 mV from the window mean,
  plus the time from first to last sample (nominally 999500 us).

These statistics identify spread and excursions; they do not discard samples
or establish whether a pulse is noise. Raw codes help distinguish ADC rail
clipping from a calibrated mV floor (such as the observed 142 mV at GND).
A quiet reading at GND alone does not prove the biased input is equally quiet.

`dump` captures the next full window and prints its first 200 chronological
samples (about 100 ms). `dump full` prints all 2000 samples to catch rare spikes.
Rows are `SAMPLE,index,t_us,raw,mV`; timestamps are relative to acquisition start,
recorded immediately before each ADC read. Raw and mV refer to the SAME read.
The firmware pauses after a dump. `resume` restarts; `pause` stops regular output.
`diag off` disables extra statistics. Diagnostic controls are session-only;
sensor profile and calibration still persist in NVS.

All diagnostic output and sorting happen AFTER sampling. Serial output creates
additional gaps between windows, especially for a full dump; acquisition is not
continuous across windows. No filtering or baseline subtraction is applied.

For the assembled Head use the setup below.
The host calculation test also checks percentiles, rail/excursion counters,
constant samples and reset between windows.

## Optional direct-VBIAS comparison of GPIO34 and GPIO35

Upload the updated firmware, then enter `pause` in the serial monitor.
Move the VBIAS lead from GPIO34 to GPIO35, keeping the rest of the circuit unchanged.
Enter `pin 35`, then `dump full`. The dump also prints the selected pin and
sensor settings. It captures even while paused, then pauses again.
For a repeat on GPIO34, move the lead back while paused, enter `pin 34`, then
`dump full`. Only the selected input is sampled; neither pin drives the circuit.
The pin command does not change the sensor profile or calibration factor.

## Current setup: assembled Head on Mini ADC2

Port pin order: 1 GND, 2 unused by Head (Mini supplies 5 V here),
3 signal / GPIO35, 4 regulated 3.3 V. Power the Head only from 3.3 V.
Use the normal Head ADC output, with R1-R4 10 kOhm, C1 10 nF and C2 10 uF.
Leave J1 open while the jack and sensor are absent; no bridge is required.
Do not add the previous breadboard's 1000-uF capacitor for the first baseline.

This Mini lab uses its own NVS namespace `sct013-mini` so the old breadboard
GPIO selection cannot silently override the initial ADC2 / GPIO35 default.
Initial sensor profile is 5 A / 0.333 V, correction 1; settings then persist.
`port adc2` selects GPIO35; `port adc1` selects GPIO34. `pin 35` and `pin 34`
remain accepted. Status and every dump identify the actual selected Mini port.

After uploading (replaces the existing Mini firmware), enter in the monitor:

```text
pause
port adc2
status
diag on
dump full
```

Repeat `dump full` for three captures without rewiring. The absent sensor means
current_A is only converted noise, not a measured load current. Compare offset,
ac_rms_mV, RAW/NOISE statistics and sample timestamps with the earlier tests.
For later sensor use select `sensor 333` or `sensor 1000` and check calibration.

Serial ports are auto-detected; select an explicit upload/monitor port in
PlatformIO if multiple devices are connected. No upload is performed by building.

EnvNode-owned source and documentation follow the repository LICENSE
(LicenseRef-EnvNode-Use-and-Redistribution-1.0).
