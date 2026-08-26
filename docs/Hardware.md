# EnvNode Hardware Documentation

This page is the entry point for the EnvNode hardware documentation and KiCad reference designs.

## Specifications and Guidelines

- [EnvNode Module Interface – Hardware Specification](EnvNode_Module_Interface_Hardware_Specification.md) defines the shared electrical and mechanical expansion-module interface.
- [Hardware Design Guidelines](hardware_guidelines.md) contains general design rules used across EnvNode hardware.

## KiCad Reference Designs

- [Daughterboard Module Interface Design Block](../hardware/kicad/DesignBlocks/EnvNode_Module_Interface_Daughterboard/README.md) documents the reusable module-side connector block, its pinout, connector geometry, routing baseline, and update workflow.

## Module Templates

- [EmptyModule FullSize](../hardware/kicad/Modules/EmptyModule_FullSize/README.md) is the validated 38 mm x 64 mm starting point for full-size daughterboards.
- [EmptyModule HalfSize](../hardware/kicad/Modules/EmptyModule_HalfSize/README.md) is the corresponding 38 mm x 32 mm starting point for compact daughterboards. Its geometry, routing, design-block linkage, and ERC have been checked; PCB-editor DRC remains a required release step.

## Module Implementations

- [DuoRelay FullSize](../hardware/kicad/Modules/DuoRelay/README.md) provides two GPIO-controlled changeover relays. It documents the relay drivers, intentionally mirrored terminals, released current limits, 250 VAC routing constraints, 8 mm SELV clearance rule, mechanical integration requirements, and bring-up procedure.
