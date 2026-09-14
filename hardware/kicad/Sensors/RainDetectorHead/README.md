# EnvNode RainDetector Head — Revision 0.1

Passive capacitive sensor head with two soldermask-covered interdigitated
F.Cu electrodes and twenty 15-ohm 1206 heater resistors on B.Cu.
The five parallel strings of four resistors give 12 ohms, approximately
417 mA and 2.08 W at 5 V (0.104 W per resistor). Select resistors rated at
least 0.25 W and check their temperature derating.

The controller and head are separate fabrication projects with independent
revisions. This head is paired with RainDetectorController revision 0.3.
The old `../Raindetector/` project is the earlier Stall.biz adapter prototype.

## Interface

| Head contact | Function | Controller contact, nominal assembly |
|---|---|---|
| J1.1 | HEATER+ | J1.1 |
| J1.4 | GND / electrode 2 | J1.4 |
| J2.1 | HEATER- | J2.4 |
| J2.4 | SENSE / electrode 1 | J2.1 |
| J1/J2 pins 2 and 3 | Unconnected, mechanical support | Unconnected |

Both boards face B.Cu toward each other. The SMD header contact axes, not
its alternating solder-pad centers, determine the mating positions.
A 180-degree head rotation exchanges the heater terminals and electrodes;
the passive elements remain functional. Equal measured baseline frequency
in both orientations is not guaranteed because of parasitic capacitances.
Confirm the actual header/socket mating height from the selected parts.

## Sensor and fabrication

C1 represents manufactured copper, not a component to purchase or solder.
The project-local footprint table resolves the sensor footprint from
`../../Libraries/EnvNode-Footprints.pretty`.
The custom design rule permits 0.10 mm clearance only between objects of C1;
other copper keeps its normal clearance requirements.

The sensor pads have no F.Mask or F.Paste openings. The current two vias
are front-plugged and front-tented (Type IV-a intent), NOT filled and capped:
`capping` and `filling` are disabled. Confirm the plugging process and
water exclusion with the fabricator; mask tenting alone is not a seal guarantee.
Any change to Type VII needs explicit fabrication settings and ordering notes.
The nominal 37.25 mm sensor height excludes the connection lobes; including
those, the copper height is about 40.27 mm.

Four 3.2 mm non-plated mounting holes are provided. Confirm screw/washer
clearance, mounting insulation and assembled spacing before manufacture.

## Validation

See `../RainDetectorController/README.md` for the shared validation commands.
ERC/DRC are design checks, not hardware or environmental qualification.
The assembled coated head has not yet been manufactured or tested.
