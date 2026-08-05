# Rain Detector

## Purpose

Detect precipitation and provide controlled heating for the external
Rain Detector Head.

The Rain Detector consists of two independent subsystems:

- rain sensing
- heater control

These subsystems are electrically independent.

---

# Rain Detector Head

The external Rain Detector Head contains:

- sensing electrodes
- integrated heater resistors

The controller electronics are located on the WeatherStation mainboard.

---

# Heater

## Status

Accepted

### Supply

5 V

### Controller

ESP32 PWM output

### Driver

Logic level N-channel MOSFET

Preferred device:

AO3400A (or equivalent)

### Gate

100 Ω series resistor

100 kΩ pull-down resistor

### Load

STALL Rain Detector Head heater

Expected resistance:

approximately 35 Ω

(to be verified on production hardware)

### PWM

0 … 100 %

The heater shall support:

- drying after rainfall
- anti-dew heating
- configurable duty cycle

No flyback diode is required.

Reason:

The heater is an ohmic load.

---

# Rain Measurement

## Status

Experimental

The sensing method is intentionally left open until validation on the
breadboard.

Experiment A

- resistive measurement
- ADS1115
- configurable reference resistor

Experiment B

- capacitive measurement using ESP32 GPIO

Experiment A shall be evaluated first.

Experiment B will only be considered if Experiment A does not provide
sufficient sensitivity.

---

# Firmware

The firmware shall always publish the raw measurement.

Examples:

weather/rain/raw

weather/rain/heater

The interpretation of the raw value is intentionally separated from the
measurement.

---

# Architecture

The Rain Detector follows ADR-0001.

Measure first.

Interpret later.