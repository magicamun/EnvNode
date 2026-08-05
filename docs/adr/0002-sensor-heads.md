# ADR-0002: External Temperature/Humidity Sensor Head

## Status

Accepted

## Context

WeatherStation requires accurate measurement of ambient air temperature and relative humidity.

The temperature/humidity sensor must therefore be physically separated from the controller electronics and mounted in a suitable radiation and weather shield.

Integrating the bare sensor directly onto the main controller PCB would expose the measurement to unwanted thermal influence from:

- ESP32
- voltage regulation
- other electronic components
- enclosure temperature

Commercially available SHT4x modules and cable assemblies already provide the required sensor electronics and expose a simple I²C interface:

- VCC
- GND
- SDA
- SCL

The required measurement accuracy may be satisfied by different members of the SHT4x family.

The exact sensor variant may therefore be selected based on required accuracy, availability and cost.

## Decision

Ambient temperature and humidity shall be implemented as an external Sensor Head.

The Sensor Head shall use a complete, factory-assembled SHT4x-based module or cable sensor.

The controller mainboard shall provide the required I²C connection and power supply.

The Sensor Head shall be mechanically positioned independently from the controller electronics.

The initial sensor variant may be SHT40, SHT41 or SHT45.

Selection of the exact variant is a component decision and does not change the system architecture.

## Rationale

The external Sensor Head is required primarily because of measurement quality and mechanical placement.

The decision is not a general rule that all sensors shall be external modules.

Sensor integration is decided individually according to the physical measurement task.

For ambient temperature and humidity:

- physical separation from heat-generating electronics is beneficial
- the sensor requires exposure to ambient air
- the existing weather/radiation shield can be reused
- commercial cable sensors avoid the need for a custom sensor PCB
- replacement and future sensor upgrades remain simple

## Consequences

The mainboard exposes a standardized four-wire I²C interface for the temperature/humidity Sensor Head:

- 3.3 V
- GND
- SDA
- SCL

The specific SHT4x sensor variant is not hard-wired into the mechanical architecture.

Firmware may use a common SHT4x driver where technically possible.

Other sensors are evaluated separately.

For example, the BMP390 pressure sensor may remain directly integrated on the mainboard because its mechanical and measurement requirements differ.

## Principle

Sensor placement follows the physical measurement requirement.

External Sensor Heads are used where spatial separation provides a technical benefit, not as a general integration rule.