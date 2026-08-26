# EnvNode AnalogInput HalfSize

## Purpose

`AnalogInput` is a HalfSize EnvNode daughterboard for one 4–20 mA current-loop pressure probe. It supplies the probe from the module's `24V` rail, converts the loop current into a voltage across a precision 150 ohm burden resistor, filters and clamps the measurement signal, and digitizes it with an ADS1115 on the system I2C bus.

The module is derived from `EmptyModule_HalfSize` and uses the standardized daughterboard interface design block.

## Project Files

- `AnalogInput.kicad_pro`
- `AnalogInput.kicad_sch`
- `AnalogInput.kicad_pcb`

## Module Interface

J1 and J2 are bottom-mounted module receptacles using:

`EnvNode-Footprints:PinSocket_2x04_P2.54mm_Vertical_Bottom`

| Pin | Net | AnalogInput use |
| ---: | --- | --- |
| 1 | `GND` | Circuit reference and current-return reference |
| 2 | `24V` | Supplies the external current-loop probe through J3 |
| 3 | `3V3` | Supplies U1 and the input-clamp rail |
| 4 | `5V` | Passed through between J1 and J2; unused by the measurement circuit |
| 5 | `SDA` | ADS1115 I2C data |
| 6 | `SCL` | ADS1115 I2C clock |
| 7 | `GPIO1` | Passed through between J1 and J2; unused by the measurement circuit |
| 8 | `GPIO2` | Passed through between J1 and J2; unused by the measurement circuit |

The completed module does not source any shared power rail. The mainboard remains responsible for rail generation, protection, and power budgeting.

## Probe Connection and Signal Path

J3 is the two-wire probe connector:

| J3 pin | Net | Function |
| ---: | --- | --- |
| 1 | `24V` | Probe supply |
| 2 | `LOOP_RETURN` | Probe current return and measurement input |

The intended probe is polarity independent. Other 4–20 mA transmitters may be polarity sensitive and must be checked before connection.

The measurement path is:

1. The probe current returns through `LOOP_RETURN`.
2. R1, 150 ohm, converts 4–20 mA into approximately 0.6–3.0 V.
3. R2, 4.7 kohm, and C1, 1 uF, form a low-pass filter with a nominal cutoff of approximately 34 Hz.
4. D1, BAT54S, clamps the ADC input toward GND and 3V3.
5. U1, ADS1115IDGS, measures the filtered signal on AIN0.

At 20 mA, R1 dissipates approximately 60 mW. The specified part is 150 ohm, 0.1 %, no more than 25 ppm/K, and rated above 0.125 W. Changing R1 changes the measurement range and requires corresponding firmware calibration and input-range review.

C2, 100 nF, is the local ADS1115 supply decoupling capacitor between 3V3 and GND.

AIN1, AIN2, AIN3, and `ALERT/RDY` are intentionally not connected.

## I2C Address Selection

The ADS1115 address is selected by closing exactly one solder jumper:

| Jumper | ADDR connection | I2C address |
| --- | --- | --- |
| JP1 | GND | `0x48` |
| JP2 | 3V3 | `0x49` |
| JP3 | SDA | `0x4A` |
| JP4 | SCL | `0x4B` |

All four jumpers are open in the PCB design. **Exactly one jumper must be solder-closed before the module is powered.** Leaving all jumpers open leaves the ADDR input undefined. Closing more than one jumper can short system nets together and must not be done.

The PCB silkscreen states `ADDR: Close exactly One` and labels all four address choices.

## Firmware Requirements

The firmware must:

- use the EnvNode system I2C bus;
- use the address selected by JP1–JP4;
- configure ADS1115 AIN0 as the measurement input;
- configure a full-scale range of at least plus/minus 4.096 V; and
- convert the measured burden voltage to loop current using the actual calibrated R1 value.

The ADS1115 power-up default range of plus/minus 2.048 V is not suitable for the complete 4–20 mA range with a 150 ohm burden resistor. It would clip above approximately 13.7 mA.

The nominal conversion is:

```text
loop_current_A = measured_voltage_V / 150 ohm
```

Pressure conversion depends on the connected probe's specified pressure range and transfer characteristic and is therefore not fixed by this module.

## Bring-up Checklist

1. Inspect U1 and D1 orientation and all solder joints.
2. Verify that exactly one of JP1–JP4 is closed and confirm the resulting I2C address.
3. Verify R1 is the specified precision 150 ohm part.
4. Power the module from current-limited SELV supplies without the probe connected.
5. Confirm 3V3 at U1 and verify the ADS1115 is detected at the selected address.
6. Configure the ADS1115 for at least the plus/minus 4.096 V range.
7. Apply a known loop current or current-loop simulator at J3.
8. Verify approximately 0.6 V at 4 mA, 1.8 V at 12 mA, and 3.0 V at 20 mA.
9. Verify the reported current and pressure conversion across the intended operating range.
10. Refill zones and rerun ERC and DRC before release.

## Manufacturing and Assembly Status

The production BOM and assembly documentation are not yet released. Component manufacturer part numbers, procurement alternatives, assembly drawings, polarity and orientation views, and inspection criteria must be completed and reviewed before assembling a production batch.

## Validated Design State

The design was checked on 2026-08-26 with KiCad 10:

- schematic ERC: no errors or warnings;
- PCB DRC: no violations;
- no unconnected pads or footprint errors;
- no global schematic labels;
- ADS1115 supply decoupling present;
- four intentionally open and correctly labeled address-selection jumpers;
- one linked daughterboard-interface design-block group;
- no routing vias; and
- successful top-side and bottom-side 3D construction and visual review.

This validation covers schematic consistency and the encoded two-dimensional PCB rules. Final performance requires physical bring-up, probe calibration, power-supply verification, and EMC assessment in the complete EnvNode assembly.
