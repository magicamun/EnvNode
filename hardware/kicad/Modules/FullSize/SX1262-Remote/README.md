# EnvNode FullSize SX1262 Remote Radio Module

This KiCad 10 project implements a Revision 0.3 EnvNode FullSize radio daughterboard using the complete EBYTE E22-900M22S module. The E22 contains a Semtech SX1262 transceiver and supports the 868 MHz and 915 MHz ISM bands. The regional frequency and radio parameters are selected in firmware.

The board uses the 38 mm × 64 mm FullSize outline, bottom-mounted 2 × 7 THT module connector, two M2.5 support holes and the standard module-identification EEPROM circuit.

## Host interface

The E22 is connected to the EnvNode host as follows:

| E22 signal | EnvNode signal | Function |
| --- | --- | --- |
| MOSI | SPI_MOSI | SPI data from host |
| MISO | SPI_MISO | SPI data to host |
| SCK | SPI_SCK | SPI clock |
| NSS | SPI_CS | Active-low SPI chip select |
| DIO1 | AUX_GPIO1 | Configurable radio interrupt |
| BUSY | AUX_GPIO2 | Radio busy indication |
| NRST | PCA9536 IO0 | Active-low hardware reset |

The PCA9536 GPIO expander is connected to I2C0. I2C1 remains unused by the radio circuitry. NRST and SPI_CS each have an external 10 kΩ pull-up to +3V3_SYS.

## RF-switch control

E22 DIO2 is connected directly to TXEN and must be enabled as the SX1262 RF-switch control output in firmware (`SetDIO2AsRfSwitchCtrl`). A BC847 NPN inverter generates the complementary RXEN signal:

- DIO2/TXEN high: TXEN high, RXEN low, transmit path enabled.
- DIO2/TXEN low: TXEN low, RXEN high, receive path enabled.

The inverter uses a 1 kΩ base resistor and a 10 kΩ RXEN pull-up. This hardware interlock avoids timing-dependent RF-switch control through an I2C GPIO expander.

## Power supply

The E22 and PCA9536 operate from +3V3_SYS. Place the 100 nF ceramic bypass capacitors close to their supply pins. The E22 additionally uses a local 22 µF ceramic bulk capacitor to support its approximately 120–140 mA transmit-current peaks.

## Antenna options

Use exactly one antenna connection:

1. Connect a suitable 868/915 MHz antenna or coaxial pigtail to the IPEX/U.FL-compatible socket on the E22. In this configuration E22 pin 21 (ANT) remains deliberately unconnected.
2. Connect a quarter-wave wire monopole to E22 pin 21. For 868 MHz, start with approximately 86 mm of straight insulated wire measured from the feed point. The carrier ground plane acts as the counterpoise. Keep the wire away from the PCB, batteries, cables and metal parts, and do not use the E22 IPEX socket simultaneously.

A wire antenna is installation-dependent and should be verified in the final enclosure. For production hardware, RF impedance and radiated performance should be measured and the antenna length or matching adjusted as required.

## Design notes

- Supply and logic levels are 3.3 V; do not apply 5 V to the E22.
- Connect every E22 GND pad to a low-impedance ground plane.
- Keep the radio and antenna away from switching regulators, fast clocks and high-current traces.
- Keep SPI traces short and route any connection from ANT pin 21 as a controlled 50 Ω RF path.
- Run KiCad ERC after schematic changes and DRC after PCB changes.

See the [module-interface hardware specification](../../../../../docs/EnvNode_Module_Interface_Hardware_Specification.md) for the authoritative EnvNode connector and mechanical requirements.
