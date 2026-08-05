# Rain Heater Driver

Purpose

Drive the heater integrated into the Rain Detector Head.

Supply

5 V

Load

Rain Detector Head heater.

Expected resistance

~35 Ω
(to be verified)

Controller

ESP32 PWM GPIO

Circuit

ESP32 GPIO
    |
 100 Ω
    |
 Gate

100 kΩ
    |
   GND

        AO3400A

Drain ---- Heater ---- +5 V

Source --------------- GND

PWM

0 ... 100 %

No flyback diode required.

Reason

The heater is an ohmic load.

Status

Prototype