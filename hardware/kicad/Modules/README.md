# EnvNode Revision 0.3 Modules

This directory contains the current module templates and functional daughterboards for the EnvNode Revision 0.3 module interface. The electrical connector contract was introduced with Revision 0.2 and is retained by the current Revision 0.3 designs.

## Functional Modules

- [FullSize/AnalogHydroPressure](FullSize/AnalogHydroPressure/README.md): ADS1115-based 4–20 mA pressure input with a local 5 V to 24 V boost supply.
- [FullSize/DuoRelay](FullSize/DuoRelay/README.md): two independently controlled 5 V changeover relays.

## Starting Templates

| Size | Bottom-mounted connector |
| --- | --- |
| [FullSize/Empty-SMD](FullSize/Empty-SMD/README.md) | SMD |
| [FullSize/Empty-THT](FullSize/Empty-THT/README.md) | THT |
| [HalfSize/Empty-SMD](HalfSize/Empty-SMD/README.md) | SMD |
| [HalfSize/Empty-THT](HalfSize/Empty-THT/README.md) | THT |

New modules must follow the [module-interface hardware specification](../../../docs/EnvNode_Module_Interface_Hardware_Specification.md). Copy the appropriate template instead of modifying it in place, preserve the connector pin mapping and mechanical reference geometry, and rerun ERC and DRC after every functional or layout change.

## Release Status

Revision 0.3 is not yet released for production. Release requires a complete article list and assembly information, physical bring-up of the relevant boards, and successful operation of at least the Rain Detector heater path. A later provisioning station is intended to prepare Mainboards, Modules, and other supported boards with their identities and required initial data.
