# EnvNode Hardware Documentation

This page is the entry point for the EnvNode hardware documentation and KiCad reference designs.

## Specifications and Guidelines

- [EnvNode Module Interface – Hardware Specification](EnvNode_Module_Interface_Hardware_Specification.md) defines the shared electrical and mechanical expansion-module interface.
- [Hardware Design Guidelines](hardware_guidelines.md) contains general design rules used across EnvNode hardware.

## KiCad Reference Designs

- [Daughterboard Module Interface Design Block](../hardware/kicad/DesignBlocks/EnvNode_Module_Interface_Daughterboard/README.md) documents the reusable module-side connector block, its pinout, connector geometry, routing baseline, and update workflow.

## Module Templates

- [EmptyModule FullSize](../hardware/kicad/Modules/EmptyModule_FullSize/README.md) is the validated 38 mm x 64 mm starting point for full-size daughterboards.
- `EmptyModule_HalfSize` remains provisional until it has been updated from the validated daughterboard design block and has passed its final ERC, DRC, and geometry checks.
