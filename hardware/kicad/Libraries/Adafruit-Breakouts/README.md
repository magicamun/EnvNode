# Adafruit 254 KiCad adapter library

Symbol: `Adafruit-Breakouts:Adafruit_254_MicroSD`  
Footprint: `Adafruit-Breakouts:Adafruit_254_MicroSD`

These adapters derive from the Adafruit design by Limor Fried/Ladyada for
Adafruit Industries and are distributed under **CC BY-SA 3.0**, not the EnvNode
root license. See `LICENSE.txt` and the unmodified `UPSTREAM-README.md` for the
original license and required attribution. EnvNode adapter conversion: 2026-10-20.

## Sources

- https://www.adafruit.com/product/254
- https://learn.adafruit.com/adafruit-micro-sd-breakout-board-card-tutorial/download
- https://github.com/adafruit/MicroSD-breakout-board
- Exact source revision: `ef4cb5171b78b6e17f12aa1fd1df41dc3f50418c`
- Geometry and JP1 connectivity: `microsd.brd` (Eagle XML).

## Installation

The `SD-Card` FullSize project's local symbol and footprint tables already
register this library. In other projects register `../Adafruit-Breakouts.kicad_sym`
and `../Adafruit-Breakouts.pretty`, both under nickname `Adafruit-Breakouts`.
Search for `Adafruit_254_MicroSD` when placing a symbol; the footprint is assigned.
The existing SD-Card schematic, PCB and project files have not been modified.

## Electrical mapping

The KiCad pad numbers exactly match Adafruit JP1, not the microSD socket pins.

| Pin | Symbol name | Adafruit net / use |
| --- | --- | --- |
| 1 | 5V_IN | VCC, onboard regulator input; connect slot +5V |
| 2 | 3V3_REG | +3V3, onboard regulator output; leave open in recommended hookup |
| 3 | GND | Ground |
| 4 | CLK | SPI clock, slot pin 13 |
| 5 | DO_MISO | Card data output, slot pin 12 |
| 6 | DI_MOSI | Card data input, slot pin 11 |
| 7 | active-low CS | Slot-specific chip select, slot pin 14 |
| 8 | CD | Raw socket card-detect contact; optional, passive pin |

Use the slot's 5 V supply and 3.3 V SPI signals. Do **not** connect 3V3_REG to
+3V3_SYS: that would join the module regulator output to the mainboard supply.
The symbol deliberately models pin 2 as power output for this hookup.
CD is not a buffered logic output. Connect it to AUX_GPIO1 with a 10 kΩ
pull-up to +3V3_SYS. Adafruit documents LOW without a card and HIGH with a
card; verify polarity on the actual socket. Source:
https://learn.adafruit.com/adafruit-micro-sd-breakout-board-card-tutorial/arduino-wiring

## Mechanical definition

Top view, breakout mounted component side up above the carrier, with a vertical
1x08, 2.54 mm pitch header below the breakout. Origin is header pin 1. Either a
soldered header or mating carrier socket can use this footprint; choose actual
connector and spacer heights together, ensuring underside clearance.

- Board outline from source: 31.75 x 25.40 mm, corner radius 1.905 mm.
  This differs slightly from rounded/current catalog dimensions.
- Header pin 1 at (0, 0); pin 8 at (0, -17.78) mm.
- Carrier electrical holes: 1.0 mm drill, 1.8 mm pads; pin 1 rectangular.
- Four 2.2 mm non-plated carrier holes at the source mounting-hole centers.
  These are #2-size clearance holes, **not M2.5 holes**.
- F.Fab and F.SilkS: source module outline; F.CrtYd: module envelope + >=0.5 mm.
- Dwgs.User: 4.5 mm screw-head envelopes and a **suggested 10 mm handling zone**
  outside the card-entry edge (left). This is not a measured inserted-card outline,
  enforced keepout or guarantee of sufficient finger clearance.
- No carrier board edge cuts, copper keepout or 3D model are included.

Do not place tall components under the breakout. Check header/socket body,
spacer height, insertion travel and enclosure access before routing. The module
fits inside the HalfSize rectangle by outer dimensions only; slot connector and
support-hole collisions still require checking in the carrier design. Compare
an actual purchased board or 1:1 print before manufacturing, particularly if its
revision differs from the source CAD.

## Validation

Both libraries were parsed and exported to SVG with the installed KiCad CLI.
Pin sequence, pitch, board outline and mounting-hole coordinates were checked
against the source Eagle XML. Carrier ERC/DRC and final mechanical placement
remain part of the HalfSize module design.
