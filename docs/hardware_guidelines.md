# Hardware Design Guidelines

## General

- Hierarchical sheets are used to separate functional blocks.
- Hierarchical pins define explicit interfaces between parent and child sheets.
- Global labels are reserved for intentionally project-wide nets; local labels remain sheet-scoped.
- One functional responsibility per sheet.

## Power

- External supply is 5 V.
- Logic supply is 3.3 V.
- Every IC shall have a local 100 nF decoupling capacitor.

## I²C

- Exactly one effective pull-up resistor pair shall exist per physical I²C bus.
- Pull-ups are nominally 4.7 kΩ unless the electrical bus analysis requires another value.
- Pull-up ownership and placement shall be defined by the system design; modules shall not add pull-ups unless the module interface explicitly permits it.
- Shared bus architecture.

## Analog

- ADS1115 is the common analog front-end.
- Sensor circuits shall not connect directly to the ESP32 ADC.
