# WeatherStation

## Vision

WeatherStation is an open-source weather sensor platform designed for home automation.

The goal is to build a transparent, modular and fully open weather sensor platform that can serve as a reliable data source for Home Assistant and other automation systems.

Instead, WeatherStation focuses on accurate sensor measurements, transparent processing and a clean separation of responsibilities.

The ESP32 acts as a measurement node.

It acquires physical sensor data, performs only the hardware-related processing required to obtain reliable measurements and publishes the results via MQTT.

Higher-level calculations such as daily statistics, sunshine duration, rain aggregation, evapotranspiration (ETo) or weather interpretation intentionally remain outside the firmware.

This architecture keeps the firmware small, deterministic and maintainable while allowing external software to evolve independently.

## Why another weather station?

The project originated from practical experience with an existing DIY weather station.

While the hardware proved to be reliable, several software aspects became increasingly problematic over time:

- closed source firmware
- undocumented algorithms
- incorrect calculation of sunshine duration
- rain aggregation inside the firmware
- difficult extensibility
- no clear separation between measurement and interpretation

Rather than replacing individual algorithms, the decision was made to redesign the entire firmware architecture around a simple principle:

## Core Philosophy

Measure first.

Interpret later.

The firmware prefers publishing raw measurements over derived values.

## Project Goals

- Fully open source firmware
- Open hardware (KiCad)
- MQTT based communication
- Web based configuration
- OTA firmware updates
- Modular software architecture
- High quality raw sensor data
- Long-term maintainability
- Easy extensibility
- Well documented hardware and software
- Vendor independant hardware

## Design Principles

The project follows a number of architectural principles.

### Observable behaviour

Whenever possible, internal decisions shall be visible through logging, diagnostics or MQTT.

The firmware should never behave like a black box.

### Measure, don't interpret

The firmware shall primarily measure physical quantities.

Derived values belong to higher software layers.

### Separation of concerns

Sensor drivers, communication, configuration and business logic are independent modules.

### Keep the firmware deterministic

No complex weather models are executed on the ESP32.

### Configuration instead of source code modifications

Whenever possible, behaviour shall be configurable.

### Hardware independence

The software shall depend on abstract interfaces rather than concrete sensor implementations.

Replacing a sensor should require only a new driver, not changes to the application logic.

## Non Goals

WeatherStation intentionally does not:

- perform weather forecasting
- calculate evapotranspiration (ETo)
- calculate sunshine duration
- aggregate historical weather data
- store long-term measurements
- replace Home Assistant

These responsibilities belong to higher software layers.

> **Status**
>
> This project is currently under active development.
>
> The architecture is considered stable, while hardware and firmware are evolving incrementally.