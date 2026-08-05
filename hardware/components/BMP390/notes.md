# BMP390

## Purpose

Digital barometric pressure and temperature sensor.

Used to measure:

- atmospheric pressure
- sensor temperature

Communication:

- I²C

SPI support is intentionally not used.

---

# Documents

Datasheet

BMP390 Datasheet Revision x.x

Reference Design

Bosch Typical Application Circuit

Breakout

<Hersteller>

---

# Electrical Connections

## Power

VDD

3.3 V

VDDIO

3.3 V

Both supply pins require local decoupling capacitors.

### Decoupling

100 nF

between VDD and GND

100 nF

between VDDIO and GND

---

# Interface

Interface

I²C

SPI intentionally disabled.

---

## SDA

Connected to common I²C bus.

---

## SCL

Connected to common I²C bus.

---

## CSB

Connected permanently to 3.3 V.

Reason:

Only I²C operation is required.

No runtime interface switching is needed.

---

## SDO

Connected to GND.

Resulting I²C address:

0x76

Only one BMP390 is expected on the bus.

Address selection jumper not required.

---

## INT

Not connected.

Reason:

Polling is sufficient for the intended measurement interval.

A test pad may optionally be provided.

---

# Shared Components

The following components are intentionally shared by the complete I²C bus.

Not per sensor.

- SDA pull-up
- SCL pull-up

Reason:

There shall only be one pull-up pair on the complete bus.

---

# Components required on PCB

BMP390

2 × 100 nF

---

# Components intentionally omitted

No voltage regulator

No level shifter

No address jumper

No SPI header

No interrupt header

---

# KiCad Status

Not yet implemented.
