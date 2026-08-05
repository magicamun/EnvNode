# Hardware Design Guidelines

## General

- Hierarchical sheets are used to separate functional blocks.
- Global labels are used instead of hierarchical pins.
- One functional responsibility per sheet.

## Power

- External supply is 5 V.
- Logic supply is 3.3 V.
- Every IC shall have a local 100 nF decoupling capacitor.

## I²C

- Exactly one pull-up resistor pair shall exist.
- Pull-ups are 4.7 kΩ.
- Shared bus architecture.

## Analog

- ADS1115 is the common analog front-end.
- Sensor circuits shall not connect directly to the ESP32 ADC.